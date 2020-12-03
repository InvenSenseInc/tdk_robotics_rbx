/*=============================================================================
  @file sns_client_example.cpp

  Example client written in C++, using libssc.  Client requests Accel SUID,
  and subsequently sends an enable request to it.  Data will stream for 10
  seconds.

  Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ===========================================================================*/

/*=============================================================================
  Include Files
  ===========================================================================*/
#include <iostream>
#include <cinttypes>
#include <unistd.h>
#include "ssc_suid_util.h"
#include <string>
#include <unordered_map>
#include <vector>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<math.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<getopt.h>
#include<signal.h>
#include<time.h>
#include<linux/types.h>

#include "sns_std_sensor.pb.h"
#include "sns_std_type.pb.h"
#include "sns_client.pb.h"
using namespace std;

/*=============================================================================
  Macro Definitions
  ===========================================================================*/

#ifndef UNUSED_VAR
#define UNUSED_VAR(var) ((void)(var));
#endif

#if 0
#define SENSOR_NAME "accel"
#define TEST_LENGTH 20  // 10 Seconds
#define SAMPLE_RATE 25  // 10 Hz
#define BATCH_PERIOD 0  // 0 ms
#endif
static int total_samples_rxved = 0;
static int sensor_id  = 0;

/*=============================================================================
  Static Data
  ===========================================================================*/

static ssc_connection *connection;
char   SENSOR_NAME[128];  
uint8_t TEST_LENGTH;
uint8_t SAMPLE_RATE;
uint8_t BATCH_PERIOD;

/*=============================================================================
  Static Function Definitions
  ===========================================================================*/
#define SENSOR_LOG_FILE     "motion_sensor.csv"
static FILE *sensor_log_fp = NULL;
static int init_log_file(void)
{
    int ret = 0;
    char log_file_name[1024];
    time_t current_time = time(NULL);
    struct tm *current_tm = localtime(&current_time);

    ret = snprintf(log_file_name,sizeof(log_file_name),"%02d.%02d.%02d_%02d.%02d.%02d-%s",
                   current_tm->tm_year+1900,current_tm->tm_mon+1,current_tm->tm_mday,
				   current_tm->tm_hour,current_tm->tm_min,current_tm->tm_sec,
				   SENSOR_LOG_FILE);
    if (ret < 0 || ret >= (int)sizeof(log_file_name)) {
        return -1;
    }

    sensor_log_fp = fopen(log_file_name,"w+");

    if(sensor_log_fp == NULL)
    {
       ret = errno;
       printf("Error opening file for logging:%s\n",strerror(errno));
       return ret;
    }

    if (fprintf(sensor_log_fp, "# Chirp Microsystems redswallow Data Log\n") < 0) {
               printf("Failed to write to %s\n", SENSOR_LOG_FILE);
               ret = -errno;
    }

    if (fprintf(sensor_log_fp, "# Non-Chirp Sensors\n") < 0) {
                   printf("Failed to write to %s\n", SENSOR_LOG_FILE);
                   ret = -errno;
    }

    if (fprintf(sensor_log_fp, "# SENSOR_RAW_ACCELEROMETER (SensorID=1)\n") < 0) {
                   printf("Failed to write to %s\n", SENSOR_LOG_FILE);
                   ret = -errno;
    }

    if (fprintf(sensor_log_fp, "# SENSOR_RAW_GYROSCOPE (SensorID=2)\n") < 0) {
                   printf("Failed to write to %s\n", SENSOR_LOG_FILE);
                   ret = -errno;
    }


    if (fprintf(sensor_log_fp, "# time[s], SensorID, x, y, z\n") < 0) {
                   printf("Failed to write to %s\n", SENSOR_LOG_FILE);
                   ret = -errno;
    }

    return ret;
}

/**
 * Event callback function, as registered with ssc_connection.
 */
static void
event_cb(const uint8_t *data, size_t size, uint64_t ts)
{
  sns_client_event_msg pb_event_msg;
  UNUSED_VAR(ts);

  sns_logv("Received QMI indication with length %zu", size);

  pb_event_msg.ParseFromArray(data, size);
  for(int i = 0; i < pb_event_msg.events_size(); i++)
  {
    const sns_client_event_msg_sns_client_event &pb_event= pb_event_msg.events(i);
	printf("Event[%i] msg_id=%i, ts=%llu", i, pb_event.msg_id(),
        (unsigned long long)pb_event.timestamp());
    sns_logv("Event[%i] msg_id=%i, ts=%llu", i, pb_event.msg_id(),
        (unsigned long long)pb_event.timestamp());	

    if(SNS_STD_MSGID_SNS_STD_ERROR_EVENT == pb_event.msg_id())
    {
      sns_std_error_event error;
      error.ParseFromString(pb_event.payload());

      sns_loge("Received error event %i", error.error());
    }
    else if(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT == pb_event.msg_id())
    {
      sns_std_sensor_physical_config_event config;
      config.ParseFromString(pb_event.payload());

      sns_loge("Received config event with sample rate %f", config.sample_rate());
    }
    else if(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT == pb_event.msg_id())
    {
      sns_std_sensor_event event;
      event.ParseFromString(pb_event.payload());
	  printf("Received sample <%f, %f, %f>",
          event.data(0), event.data(1), event.data(2));
      sns_logi("Received sample <%f, %f, %f>",
          event.data(0), event.data(1), event.data(2));
      if(sensor_log_fp != NULL)
         fprintf(sensor_log_fp,"%llu,%d,%f,%f,%f\n",(unsigned long long)pb_event.timestamp(),sensor_id,event.data(0), event.data(1), event.data(2));
      total_samples_rxved++;
    }
    else
    {
      printf("Received unknown message ID %i", pb_event.msg_id());
	  sns_loge("Received unknown message ID %i", pb_event.msg_id());
    }
  }
}

/**
 * Send a SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG to SUID
 */
static void
send_config_req(ssc_connection *conn, sensor_uid const *suid)
{
  string pb_req_msg_encoded;
  string config_encoded;
  sns_client_request_msg pb_req_msg;
  sns_std_sensor_config config;

  sns_logi("Send config request with sample rate %i and batch period %d",
                SAMPLE_RATE, BATCH_PERIOD);

  config.set_sample_rate(SAMPLE_RATE);
  config.SerializeToString(&config_encoded);

  pb_req_msg.set_msg_id(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG);
  pb_req_msg.mutable_request()->set_payload(config_encoded);
  pb_req_msg.mutable_suid()->set_suid_high(suid->high);
  pb_req_msg.mutable_suid()->set_suid_low(suid->low);
  pb_req_msg.mutable_susp_config()->
    set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
  pb_req_msg.mutable_susp_config()->
    set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);
  pb_req_msg.mutable_request()->mutable_batching()->
    set_batch_period(BATCH_PERIOD);
  pb_req_msg.SerializeToString(&pb_req_msg_encoded);
  conn->send_request(pb_req_msg_encoded);
}

/**
 * SUID callback as registered with suid_lookup.
 */
static void
suid_cb(const std::string& datatype, const std::vector<sensor_uid>& suids)
{
  sns_logv("Received SUID event with length %zu", suids.size());
  if(suids.size() > 0)
  {
    sensor_uid suid = suids.at(0);
    connection = new ssc_connection(event_cb);

    printf("Received SUID %" PRIx64 "%" PRIx64 " for '%s' \n",
        suid.high, suid.low, datatype.c_str());
    send_config_req(connection, &suid);
  } else {
    sns_logi("%s sensor is not available", SENSOR_NAME);
    printf("%s sensor is not available \n", SENSOR_NAME);
    exit(-1);
  }
}
static void close_log_file(void)
{
   if(sensor_log_fp != NULL)
     fclose(sensor_log_fp);
}
int
main(int argc, char *argv[])
{
  int ret = 0;  
  if(argc == 5)
  {
   strcpy(SENSOR_NAME,argv[1]);
   char *a = argv[2];
   TEST_LENGTH = atoi(a);
   a = argv[3];
   SAMPLE_RATE = atoi(a);
   a = argv[4];
   BATCH_PERIOD = atoi(a);
  }  
  ret = init_log_file();

  if(ret < 0)
  {
    printf("Failed to init log file\n");
  }

  printf("##################################################################################################\n");
  printf("streaming started for '%s' set SR/RR '(%d/%d)Hz' and duration '%dSec' \n", SENSOR_NAME,
                            SAMPLE_RATE, BATCH_PERIOD, TEST_LENGTH);
  sns_logi("streaming started for '%s' set SR/RR '(%d/%d)Hz' and duration '%dSec' ", SENSOR_NAME,
                            SAMPLE_RATE, BATCH_PERIOD, TEST_LENGTH);
  
  if(strcmp(SENSOR_NAME,"accel") == 0)
  {
    sensor_id = 1;
  }
  else if (strcmp(SENSOR_NAME,"gyro") == 0)
  {
    sensor_id = 2;
  }

  suid_lookup lookup(suid_cb);
  lookup.request_suid(SENSOR_NAME);
  sleep(TEST_LENGTH);
  delete connection;
  printf("Received %d samples for '%s' sensor, set SR/RR '(%d/%d)Hz' and duration '%dSec'", total_samples_rxved, SENSOR_NAME,
                            SAMPLE_RATE, BATCH_PERIOD, TEST_LENGTH);

  
  printf("##################################################################################################\n");
  close_log_file();
  return 0;
}

