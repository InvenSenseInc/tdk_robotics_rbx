/*
 * Copyright (c) 2020-2021 InvenSense, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "invn_algo_rangefinder.h"

#define DEV_NUM_BOUNDARY 3
#define TX_RX_MODE   0x10
#define RX_ONLY_MODE   0x20

long long orig_buffer[32];

int sensor_connected[] = {0, 0, 0, 0, 0, 0};
uint32_t op_freq[] = {0, 0, 0, 0, 0, 0};


/*! \struct InvnRangeFinder
 * InvnRangeFinder data structure that store internal algorithm state.
 * The struct below shows one method to align the data buffer pointer to
 * 32 bit for 32bit MCU. Other methods can be used to align memory using
 * malloc or attribute((aligned, 4))
 */
union InvnRangeFinder {
	uint8_t data[INVN_RANGEFINDER_DATA_STRUCTURE_SIZE];
	uint32_t data32;
} InvnRangeFinder;


#define CHIRP_NAME		"ch101"
#define IIO_DIR		"/sys/bus/iio/devices/"
#define FIRMWARE_PATH		"/usr/share/tdk/"
#define MAX_SYSFS_NAME_LEN	(100)
#define MAX_NUM_SAMPLES     450
#define CH101_DEFAULT_FW       2
#define CH201_DEFAULT_FW       5

#define VER_MAJOR (0)
#define VER_MINOR (6)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static const char  *const fw_names[] = {
"v39.hex",
"v39_IQ_debug-02.hex",
"ch101_gpr_rxopt_v41b.bin",
"ch101_gpr_rxopt_v41b-IQ_Debug.bin",
"ch201_old.bin",
"ch201_gprmt_v10a.bin",
"ch201_gprmt_v10a-IQ_Debug.bin",
};

static char *firmware_path;

static char sysfs_path[MAX_SYSFS_NAME_LEN] = {0};
static char dev_path[MAX_SYSFS_NAME_LEN] = {0};
static char sensor_connection[6];

static int get_lib_range(uint32_t fop, int16_t *iq_buffer, int mode,
	int samples, unsigned short *distance, unsigned short *amplitude)
{
	int res;

	InvnAlgoRangeFinderConfig config;
	InvnAlgoRangeFinderInput inputs;
	InvnAlgoRangeFinderOutput outputs;
	int nb_skip_sample_pitch_catch;

	union InvnRangeFinder algo;

	invn_algo_rangefinder_generate_default_config(&config);
	if (mode == TX_RX_MODE) {
		config.sensor_FOP = fop; // update config FOP to the sensor FOP
		invn_algo_rangefinder_init(&algo, &config);
		inputs.time = 0;
		inputs.Tx = 0;
		inputs.Rx = 0;
		inputs.nbr_samples_skip = 0;
	} else {
		// pitch_catch example
		// update config FOP to the receiving sensor FOP
		config.sensor_FOP = fop;
		config.pre_trigger_time = 0; // update value of pre-trigger time
		// update distance between center of sensors
		config.inter_sensor_distance_mm = 28;
		invn_algo_rangefinder_init(&algo, &config);
	// ask for corresponding samples to skip due to pre-trigger_time,
	//help for integrator no requirement. Skip value is not mandatory
		nb_skip_sample_pitch_catch =
			invn_algo_rangefinder_convert_time_to_nbr_samples(
			config.pre_trigger_time, config.sensor_FOP);
		inputs.time = 0;
		inputs.Tx = 0;
		inputs.Rx = 1;
		inputs.nbr_samples_skip = 0;
	}
	inputs.nbr_samples = samples;
	inputs.iq_buffer = iq_buffer;
	invn_algo_rangefinder_process(&algo, &inputs, &outputs);

	*distance = outputs.distance_to_object;
	*amplitude = outputs.magnitude_of_echo;

	return res;
}

int inv_load_dmp(char *dmp_path, int dmp_version,
	const char *dmp_firmware_path)
{
	int result = 0;
	int bytesWritten = 0;
	FILE *fp, *fp_read;
	int write_size, len;
	char firmware_file[100];
	char bin_file[100];
	char bank[256];
	const char *p;
	int fw_names_size = ARRAY_SIZE(fw_names);

	snprintf(firmware_file, 100, "%s/misc_bin_dmp_firmware_vers",
		dmp_path);
	fp = fopen(firmware_file, "wb");
	if (fp == NULL) {
		printf("dmp sysfs file %s open fail\n", firmware_file);
		fclose(fp_read);
		return -EINVAL;
	}

	printf("file=%s\n", firmware_file);

	if (dmp_version != 0xff)
		p = (const void *)fw_names[dmp_version];
	else
		p = (const void *)firmware_path;

	write_size = strlen(p);
	if (write_size > 30)
		write_size = 30;

	printf("input name=%s\n", p);

	bytesWritten = fwrite(p, 1, write_size, fp);
	if (bytesWritten != write_size) {
		printf(
			"bytes written (%d) not match requested length (%d)\n",
			bytesWritten, write_size);
		result = -1;
	}

	fclose(fp);

	if (dmp_version != 0xff) {
		if (result == 0)
			printf("dmp firmware %s written to %s\n",
				fw_names[dmp_version], firmware_file);

		if (dmp_version >= fw_names_size) {
			printf("dmp_version %d out of range [0-%d]\n",
				dmp_version, fw_names_size);
			return -EINVAL;
		}

		snprintf(bin_file, 100, "%s%s", dmp_firmware_path,
			fw_names[dmp_version]);
	} else {
		if (result == 0)
			printf("dmp firmware %s written to %s\n",
				firmware_path, firmware_file);
		snprintf(bin_file, 100, "%s%s", dmp_firmware_path,
			firmware_path);
	}

	fp_read = fopen(bin_file, "rb");
	if (fp_read == NULL) {
		printf("dmp firmware file %s open fail\n", bin_file);
		return -EINVAL;
	}

	fseek(fp_read, 0L, SEEK_END);
	len = ftell(fp_read);
	fseek(fp_read, 0L, SEEK_SET);
	snprintf(firmware_file, 100, "%s/misc_bin_dmp_firmware", dmp_path);

	fp = fopen(firmware_file, "wb");
	if (fp == NULL) {
		printf("dmp sysfs file %s open fail\n", firmware_file);
		fclose(fp_read);
		return -EINVAL;
	}
	while (len > 0) {
		if (len > 256)
			write_size = 256;
		else
			write_size = len;
		bytesWritten = fread(bank, 1, write_size, fp_read);
		bytesWritten = fwrite(bank, 1, write_size, fp);
		if (bytesWritten != write_size) {
			printf(
			"bytes written (%d) no match requested length (%d)\n",
				bytesWritten, write_size);
			result = -1;
		}
		len -= write_size;
	}

	fclose(fp);
	fclose(fp_read);


	return result;
}

int check_sensor_connection(void)
{
	int i;
	FILE *fp;
	char file_name[100];

	for (i = 0; i < 6; i++) {
		snprintf(file_name, 100, "%s/in_positionrelative%d_raw",
			sysfs_path, i+18);
		fp = fopen(file_name, "rt");
		if (fp == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else {
			//printf("open %s OK\n", file_name);
		}
		fscanf(fp, "%d", &op_freq[i]);
		fclose(fp);
		if (op_freq[i])
			sensor_connected[i] = 1;
		else
			sensor_connected[i] = 0;
	}

	for (i = 3; i < 6; i++)
		printf("%d, %d\n", sensor_connected[i], op_freq[i]);

	for (i = 0; i < 3; i++)
		printf("%d, %d\n", sensor_connected[i], op_freq[i]);

	return 0;
}

/**
 * find_type_by_name() - function to match top level types by name
 * @name: top level type instance name
 * @type: the type of top level instance being sort
 *
 * Typical types this is used for are device and trigger.
 **/
int find_type_by_name(const char *name, const char *type)
{
	const struct dirent *ent;
	int number, numstrlen;

	FILE *nameFile;
	DIR *dp;
	char thisname[MAX_SYSFS_NAME_LEN];
	char filename[MAX_SYSFS_NAME_LEN];
	size_t filename_sz = 0;
	int ret = 0;
	int status = -1;

	dp = opendir(IIO_DIR);
	if (dp == NULL) {
		printf("No industrialio devices available\n");
		return -EINVAL;
	}

	while (ent = readdir(dp), ent != NULL) {
		if (strcmp(ent->d_name, ".") != 0 &&
		    strcmp(ent->d_name, "..") != 0 &&
		    strlen(ent->d_name) > strlen(type) &&
		    strncmp(ent->d_name, type, strlen(type)) == 0) {

			printf("name: %s\n", ent->d_name);

			numstrlen = sscanf(ent->d_name + strlen(type),
					   "%d", &number);
			filename_sz = strlen(IIO_DIR)
					+ strlen(type)
					+ numstrlen
					+ 6;

			/* verify the next character is not a colon */
			if (strncmp(ent->d_name + strlen(type) + numstrlen,
					":", 1) != 0) {

				snprintf(filename, filename_sz, "%s%s%d/name",
					IIO_DIR, type, number);

				nameFile = fopen(filename, "r");
				if (!nameFile)
					continue;
				ret = fscanf(nameFile, "%s", thisname);
				fclose(nameFile);

				if (ret == 1 && strcmp(name, thisname) == 0) {
					status = number;
					goto exit_closedir;
				}
			}
		}
	}

exit_closedir:
	closedir(dp);
	return status;
}

static int process_sysfs_request(char *data)
{
	int dev_num = find_type_by_name(CHIRP_NAME, "iio:device");

	if (dev_num < 0)
		return -EINVAL;

	snprintf(data, 100, IIO_DIR "iio:device%d", dev_num);
	snprintf(dev_path,  100, "/dev/iio:device%d", dev_num);

	return 0;
}


int switch_streaming(int on)
{
	int i;

	FILE *fp;
	char file_name[100];

	for (i = 0; i < 6; i++) {
		snprintf(file_name, 100, "%s/scan_elements/in_proximity%d_en",
			sysfs_path, i);
		fp = fopen(file_name, "wt");
		if (fp == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else {
			//printf("open %s OK\n", file_name);
		}
		fprintf(fp, "%d", sensor_connected[i] & on);
		fclose(fp);
	}
	//add 6 for distance
	for (i = 0; i < 6; i++) {
		snprintf(file_name, 100, "%s/scan_elements/in_distance%d_en",
			sysfs_path, i+6);
		fp = fopen(file_name, "wt");
		if (fp == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else {
			//printf("open %s OK\n", file_name);
		}
		fprintf(fp, "%d", sensor_connected[i] & on);
		fclose(fp);
	}

	//add 12 for intensity
	for (i = 0; i < 6; i++) {
		snprintf(file_name, 100, "%s/scan_elements/in_intensity%d_en",
			sysfs_path, i+12);
		fp = fopen(file_name, "wt");
		if (fp == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else {
			//printf("open %s OK\n", file_name);
		}
		fprintf(fp, "%d", sensor_connected[i] & on);
		fclose(fp);
	}

	//add 18 for intensity(rx/tx id)
	for (i = 0; i < 6; i++) {
		snprintf(file_name, 100,
			"%s/scan_elements/in_positionrelative%d_en",
				sysfs_path, i+18);
		fp = fopen(file_name, "wt");
		if (fp == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else {
			//printf("open %s OK\n", file_name);
		}
		fprintf(fp, "%d", sensor_connected[i] & on);
		fclose(fp);
	}

	snprintf(file_name, 100, "%s/scan_elements/in_timestamp_en",
		sysfs_path);
	fp = fopen(file_name, "wt");
	if (fp == NULL) {
		printf("error opening proximity\n");
		exit(0);
	} else {
		//printf("open proximity OK\n");
	}
	fprintf(fp, "%d", on);
	fclose(fp);

	snprintf(file_name, 100, "%s/buffer/length", sysfs_path);
	fp = fopen(file_name, "wt");
	if (fp == NULL) {
		printf("error opening length\n");
		exit(0);
	} else {
		//printf("open length OK\n");
	}
	fprintf(fp, "%d", 2000);
	fclose(fp);

	snprintf(file_name, 100, "%s/buffer/watermark", sysfs_path);
	fp = fopen(file_name, "wt");
	if (fp == NULL) {
		printf("error opening watermark\n");
		exit(0);
	} else {
	   //printf("open watermark OK\n");
	}
	fprintf(fp, "%d", 1);
	fclose(fp);

	snprintf(file_name, 100, "%s/buffer/enable", sysfs_path);
	fp = fopen(file_name, "wt");
	if (fp == NULL) {
		printf("error opening master enable\n");
		exit(0);
	} else {
		printf("open master enable OK\n");
	}
	fprintf(fp, "%d", 1 & on);
	fclose(fp);

}

int16_t I[6][MAX_NUM_SAMPLES], Q[6][MAX_NUM_SAMPLES];
int16_t iq_buffer[MAX_NUM_SAMPLES * 2];
float sample_to_mm[6];
#define CH_SPEEDOFSOUND_MPS	343
int8_t port_map[6] = {4, 5, 6, 1, 2, 3};

unsigned short distance[6], amplitude[6];
char mode[6];
void print_header(int sample, int frequency, FILE *fp)
{
	int i, j;
	float pos;

	for (i = 0; i < 6; i++) {
		if (op_freq[i]) {
			sample_to_mm[i] =
				(sample*CH_SPEEDOFSOUND_MPS * 8 * 1000)
				/ (op_freq[i] * 2);
			//printf("to_mm=%f\n", sample_to_mm[i]);
		}
	}

	fprintf(fp, "# Chirp Microsystems redswallow Data Log\n");

	fprintf(fp, "# sample rate:, %d S/s\n", frequency*sample);
	fprintf(fp, "# Decimation factor:, 1\n");
	fprintf(fp, "# Content: iq\n");
	fprintf(fp, "# Sensors ID:, ");
	for (i = 3; i < 6; i++) {
		if (sensor_connected[i])
			fprintf(fp, "%d, ", i-2);
	}
	for (i = 0; i < 3; i++) {
		if (sensor_connected[i])
			fprintf(fp, "%d, ", i+4);
	}

	fprintf(fp, "\n# Sensors FOP:, ");
	for (i = 3; i < 6; i++) {
		if (sensor_connected[i])
			fprintf(fp, "%d, ", op_freq[i]);
	}
	for (i = 0; i < 3; i++) {
		if (sensor_connected[i])
			fprintf(fp, "%d, ", op_freq[i]);
	}

	fprintf(fp, "Hz\n");

	fprintf(fp, "# Sensors NB Samples:,");
	for (i = 3; i < 6; i++) {
		if (sensor_connected[i])
			fprintf(fp, "%d, ", sample*2);
	}
	for (i = 0; i < 3; i++) {
		if (sensor_connected[i])
			fprintf(fp, "%d, ", sample);
	}

	fprintf(fp, "\n# Sensors NB First samples skipped:, 0, 0\n");
	for (j = 3; j < 6; j++) {
		if (sensor_connected[j]) {
			fprintf(fp,
"# time [s],tx_id,rx_id, range [cm],intensity [a.u.], target_detected, ");
			pos = 0;
			for (i = 0; i < sample; i++) {
				pos += sample_to_mm[j];
				fprintf(fp, "i_data_%3.1f, ", pos/1000.0);
			}
			pos = 0;
			for (i = 0; i < sample; i++) {
				pos += sample_to_mm[j];
				fprintf(fp, "q_data_%3.1f, ", pos/1000.0);
			}
			fprintf(fp, "\n");
		}
	}
	for (j = 0; j < 3; j++) {
		if (sensor_connected[j]) {
			fprintf(fp,
"# time [s],tx_id,rx_id, range [cm],intensity [a.u.], target_detected, ");
			pos = 0;
			for (i = 0; i < sample; i++) {
				pos += sample_to_mm[j];
				fprintf(fp, "i_data_%3.1f, ", pos/1000.0);
			}
			pos = 0;
			for (i = 0; i < sample; i++) {
				pos += sample_to_mm[j];
				fprintf(fp, "q_data_%3.1f, ", pos/1000.0);
			}
			fprintf(fp, "\n");
		}
	}

}

static void print_help(void)
{
	printf("Usage:\n");
	printf("-h: print this help\n");
	printf("-d x: duration,  unit in seconds. default: 10 seconds\n");
	printf("-s x: number of samples. default: 40 samples\n");
	printf("-f x: sampling frequency. Default: 5 Hz\n");
	printf("-n: not loading firmware. Default will load firmware\n");
	printf(
	"-l string: output logging file name. Default: \"/usr/chirp.csv\"\n");

}

void log_data(int index, int num_sensors, int sample, FILE *log_fp,
	long long last_timestamp)
{
	int dev_num, i, j;

	for (dev_num = 0; dev_num < num_sensors; dev_num++) {
	//only CH101 and only in RX_TX mode, we call algo to calculate
		if (((mode[dev_num] == 0x20) || (mode[dev_num] == 0x10)) &&
			(sensor_connection[dev_num] < 3)) {
			for (i = 0; i < sample; i++) {
				iq_buffer[2*i] = I[dev_num][i];
				iq_buffer[2*i+1] = Q[dev_num][i];
			}
			//for (i = 0; i < inputs.nbr_samples*2; i++) {
				//printf("%d, ", inputs.iq_buffer[i]);
			//}
			//printf("\n");

			get_lib_range(op_freq[sensor_connection[dev_num]],
				iq_buffer, mode[dev_num], sample,
				 &distance[dev_num], &amplitude[dev_num]);

		}
	}

	for (dev_num = 0; dev_num < num_sensors; dev_num++) {
		if (mode[dev_num] == 0) {
			printf("mode 0 here\n");
			break;
		}
		fprintf(log_fp, "%f, ", last_timestamp/1000000000.0);
		//printf("%f, ", last_timestamp/1000000000.0);

		//TX_RX mode
		if (mode[dev_num] == 0x10) {
			fprintf(log_fp, "%d, ",
				port_map[sensor_connection[dev_num]]);
			fprintf(log_fp, "%d, ",
				port_map[sensor_connection[dev_num]]);
		}

		//RX only mode.
		if (mode[dev_num] == 0x20) {
			for (j = 0; j < num_sensors; j++) {
				if ((mode[j] == 0x10) &&
				(sensor_connection[j] < 3)) {
					fprintf(log_fp, "%d, ",
					port_map[sensor_connection[j]]);
				}
			}
			fprintf(log_fp, "%d, ",
				port_map[sensor_connection[dev_num]]);
		}

		fprintf(log_fp, "%d, ", distance[dev_num]/10);
		fprintf(log_fp, "%d, ", amplitude[dev_num]);
		if ((distance[dev_num] == 0xFFFF) ||
			(distance[dev_num] == 0)) {
			fprintf(log_fp, "%d, ", 0);
		} else {
			fprintf(log_fp, "%d, ", 1);
		}

		if (sensor_connection[dev_num] < 3) {
			for (i = 0; i < sample; i++) {
				fprintf(log_fp, "%d, ", I[dev_num][i]);
				//printf("%d, ", I[dev_num][i]);
			}
			for (i = 0; i < sample; i++) {
				fprintf(log_fp, "%d, ", Q[dev_num][i]);
				//printf("%d, ", Q[dev_num][i]);
			}
		}
		fprintf(log_fp, "\n");
	}
}

int main(int argc, char *argv[])
{
	FILE *fp;
	FILE *log_fp;
	char *buffer;
	char file_name[100];
	int i, j, bytes;
	int total_bytes;
	int target_bytes;
	int scan_bytes;
	long long timestamp, last_timestamp;
	int counter;
	int num_sensors;
	int ready;
	int index;
	int nfds, num_open_fds;
	struct pollfd pfds[1];
	int dur, sample, freq;
	char *log_file;
	int c, fp_writes;
	int load_firmware_flag;

	buffer = (char *)orig_buffer;
	printf("\n\nTDK-Robotics-RB5-chx01-app-%d.%d\n\n",
		VER_MAJOR, VER_MINOR);

	// get absolute IIO path & build MPU's sysfs paths
	if (process_sysfs_request(sysfs_path) < 0) {
		printf("Cannot find %s sysfs path\n", CHIRP_NAME);
		exit(0);
	}

	printf("%s sysfs path: %s, dev path=%s\n",
		CHIRP_NAME, sysfs_path, dev_path);

	dur = 10;
	sample = 80;
	freq = 5;
	log_file = "/usr/chirp.csv";
	load_firmware_flag = 1;

	opterr = 0;
	while ((c = getopt(argc, argv, "hd:s:f:l:n")) != -1) {
		switch (c) {
		case 'h':
			print_help();
			return 0;
		case 'd':
			dur = atoi(optarg);
			break;
		case 's':
			sample = atoi(optarg);
			break;
		case 'f':
			freq = atoi(optarg);
			break;
		case 'l':
			log_file = optarg;
			break;
		case 'n':
			load_firmware_flag = 0;
			break;
		default:
			abort();
		}
	}
	if (freq > 10)
		freq = 10;
	if (sample > 225)
		sample = 225;

	printf(
"options, log file=%s, frequency=%d, samples=%d, duration=%d seconds\n",
	log_file, freq, sample, dur);

	printf("firmware load=%d\n", load_firmware_flag);
	if (load_firmware_flag) {
		if (inv_load_dmp(sysfs_path,
			CH101_DEFAULT_FW, FIRMWARE_PATH) != 0) {
			printf("CH101 firmware fail\n");
			return -EINVAL;
		}

		if (inv_load_dmp(sysfs_path,
			CH201_DEFAULT_FW, FIRMWARE_PATH) != 0) {
			printf("CH201 firmware fail\n");
			return -EINVAL;
		}
	}

	index = 0;
	counter = freq*dur;

	for (i = 0; i < 6; i++) {
		snprintf(file_name, 100, "%s/in_positionrelative%d_raw",
			sysfs_path, i+18);
		fp = fopen(file_name, "wt");
		if (fp == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else {
			//printf("open %s OK, with %d\n", file_name, sample);
		}
		fprintf(fp, "%d", sample);
		fclose(fp);
	}

	check_sensor_connection();

	last_timestamp = 0;
	timestamp = 0;
	scan_bytes = 0;
	num_sensors = 0;
	for (i = 0; i < 6; i++) {
		scan_bytes += sensor_connected[i]*32;
		num_sensors += sensor_connected[i];
	}
	index = 0;
	for (i = 0; i < 6; i++) {
		if (sensor_connected[i] == 1) {
			sensor_connection[index] = i;
			index++;
		}
	}

	log_fp = fopen(log_file, "wt");
	if (log_fp == NULL) {
		printf("error opening log file %s\n", log_file);
		exit(0);
	}
	print_header(sample, freq, log_fp);

	scan_bytes += 32;

	printf("counter=%d\n", counter);

	// counter controls how many times it will run.
	snprintf(file_name, 100, "%s/calibbias", sysfs_path);
	fp = fopen(file_name, "wt");
	if (fp == NULL) {
		printf("error opening %s\n", file_name);
		exit(0);
	} else {
		printf("open %s OK\n", file_name);
	}
	fprintf(fp, "%d", counter);
	fclose(fp);

	snprintf(file_name, 100, "%s/sampling_frequency", sysfs_path);

	fp = fopen(file_name, "wt");
	if (fp == NULL) {
		printf("error opening %s\n", file_name);
		exit(0);
	} else {
		printf("open %s OK\n", file_name);
	}
	fprintf(fp, "%d", freq);
	fclose(fp);

	total_bytes = 0;
	switch_streaming(1);


	pfds[0].fd = open(dev_path, O_RDONLY);
	pfds[0].events = (POLLIN | POLLRDNORM);
	last_timestamp = 0;

	ready = 1;
	fp_writes = 1;
	while (ready == 1) {

		char *tmp;
		short value;
		char *ptr;
		int dev_num;

		pfds[0].revents = 0;
		//printf("before new polling counter=%d\n", counter);
		nfds = 1;
		ready = poll(pfds, nfds, 5000);
		//printf("pass poll 0x%x, ready=%d\n", pfds[0].revents, ready);
		if (ready == -1)
			printf("poll error\n");
		if (pfds[0].revents & (POLLIN | POLLRDNORM)) {
			target_bytes = scan_bytes;
			bytes = read(pfds[0].fd, buffer, target_bytes);
			total_bytes += bytes;
			tmp = (char *)&timestamp;
			memcpy(tmp, &buffer[scan_bytes - 8], 8);
	//amplitude 2 bytes + intensity data
	//2 bytes 224/8 = 28bytes(IQ)+mode(1 bytes)
			target_bytes = scan_bytes-32;
			i = 0;
			//for (i = 0; i < 64; i++) {
				//printf("%d, ", buffer[i]);
			//}
			//printf("\n");
			if (last_timestamp == 0)
				last_timestamp = timestamp;

			if (last_timestamp != timestamp) {

				index -= 7;
				fp_writes++;
				log_data(fp_writes, num_sensors, sample, log_fp,
					last_timestamp);

				index = 0;
				last_timestamp = timestamp;
			}

			//printf("\nindex=%d\n", index);
			for (j = 0; j < num_sensors; j++) {
				for (i = 0; i < 7; i++) {
					value = buffer[i*4+1+j*28];
					value <<= 8;
					value += buffer[i*4+j*28];
					I[j][i+index] = value;

					value = buffer[i*4+3+j*28];
					value <<= 8;
					value += buffer[i*4+2+j*28];

					Q[j][i+index] = value;
					//printf("%d, %d", I[j][i], Q[j][i]);
				}
			}
			//printf("\n");
			index += 7;

			ptr = buffer + 28*num_sensors;

			for (j = 0; j < num_sensors; j++) {
				distance[j] = ptr[1+j*2];
				distance[j] <<= 8;
				distance[j] += ptr[j*2];
				//printf("distance[%d]=%d\n", j, distance[j]);
			}


			ptr += 2*num_sensors;

			for (j = 0; j < num_sensors; j++) {
				amplitude[j] = ptr[1+j*2];
				amplitude[j] <<= 8;
				amplitude[j] += ptr[j*2];
			}

			ptr += 2*num_sensors;

			for (j = 0; j < num_sensors; j++)
				mode[j] = ptr[j];
		}
	}
	switch_streaming(0);
	fclose(log_fp);

	if (fp_writes == counter)
		printf("PASS: setting=%d, get=%d\n", counter, fp_writes);
	else
		printf("FAIL: setting=%d, get=%d\n", counter, fp_writes);


	return 0;
}
