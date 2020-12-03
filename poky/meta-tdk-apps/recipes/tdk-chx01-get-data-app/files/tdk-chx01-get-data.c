#include <poll.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
 
long long orig_buffer[32];

int sensor_connected[]={0, 0, 0, 0, 0, 0};


#define CHIRP_NAME		"ch101"
#define IIO_DIR 		"/sys/bus/iio/devices/"
#define FIRMWARE_PATH		"/usr/share/tdk/"
#define MAX_SYSFS_NAME_LEN	(100)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static char sysfs_path[MAX_SYSFS_NAME_LEN] = {0};
static char dev_path[MAX_SYSFS_NAME_LEN] = {0};

int check_sensor_connection(){
	int i;
	int fd;
	FILE *fp;
	char file_name[100];
	
    for(i=0;i<6;i++){
		sprintf(file_name, "%s/in_positionrelative%d_raw", sysfs_path, i+18);
		fd = open(file_name, O_RDONLY);
		if((fp = fopen(file_name, "rt")) == NULL)
		if(fd <0){
			printf("error opening %s\n", file_name);
			exit(0);
		} else{
			//printf("open %s OK\n", file_name);
		}
		fscanf(fp, "%d", &sensor_connected[i]);
		fclose(fp);
		printf("%d\n", sensor_connected[i]);
	}	
	
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
		return -1;
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
		return -1;

	sprintf(data, IIO_DIR "iio:device%d", dev_num);
	sprintf(dev_path,  "/dev/iio:device%d", dev_num);

	return 0;
}


int switch_streaming(int on){
	int i;
	
	FILE *fp;
	char file_name[100];
	
    for(i=0;i<6;i++){
		sprintf(file_name, "%s/scan_elements/in_proximity%d_en", sysfs_path, i);
		if((fp = fopen(file_name, "wt")) == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else{
			//printf("open %s OK\n", file_name);
		}
		fprintf(fp, "%d", sensor_connected[i] & on);
		fclose(fp);
	}
	//add 6 for distance
    for(i=0;i<6;i++){
		sprintf(file_name, "%s/scan_elements/in_distance%d_en", sysfs_path, i+6);
		if((fp = fopen(file_name, "wt")) == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else{
			//printf("open %s OK\n", file_name);
		}
		fprintf(fp, "%d", sensor_connected[i] & on);
		fclose(fp);
	}

	//add 12 for intensity
    for(i=0;i<6;i++){
		sprintf(file_name, "%s/scan_elements/in_intensity%d_en", sysfs_path, i+12);
		if((fp = fopen(file_name, "wt")) == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else{
			//printf("open %s OK\n", file_name);
		}
		fprintf(fp, "%d", sensor_connected[i] & on);
		fclose(fp);
	}
	
	//add 18 for intensity(rx/tx id)
    for(i=0;i<6;i++){
		sprintf(file_name, "%s/scan_elements/in_positionrelative%d_en", sysfs_path, i+18);
		if((fp = fopen(file_name, "wt")) == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else{
			//printf("open %s OK\n", file_name);
		}
		fprintf(fp, "%d", sensor_connected[i] & on);
		fclose(fp);
	}
	
	sprintf(file_name, "%s/scan_elements/in_timestamp_en", sysfs_path);
	if((fp = fopen(file_name, "wt")) == NULL) {
		printf("error opening proximity\n");
		exit(0);
	} else{
		//printf("open proximity OK\n");
	}
	fprintf(fp, "%d", on);
	fclose(fp);
	
	sprintf(file_name, "%s/buffer/length", sysfs_path);
	if((fp = fopen(file_name, "wt")) == NULL) {
	    printf("error opening length\n");
	    exit(0);
	} else{
	   //printf("open length OK\n");
	}
	fprintf(fp, "%d", 2000);
	fclose(fp);

	sprintf(file_name, "%s/buffer/watermark", sysfs_path);
	if((fp = fopen(file_name, "wt")) == NULL) {
	    printf("error opening watermark\n");
	    exit(0);
	} else{
	   //printf("open watermark OK\n");
	}
	fprintf(fp, "%d", 1);
	fclose(fp);
	
	sprintf(file_name, "%s/buffer/enable", sysfs_path);
	if((fp = fopen(file_name, "wt")) == NULL) {
	    printf("error opening master enable\n");
	    exit(0);
	} else{
	   printf("open master enable OK\n");
	}
	fprintf(fp, "%d", 1 & on);
	fclose(fp);
	
}

short I[6][200], Q[6][200];
unsigned short distance[6], amplitude[6];
char mode[6];
void print_header(int sample, int frequency, FILE *fp){
	int i;
	
	fprintf(fp, "# Chirp Microsystems redswallow Data Log\n");

	fprintf(fp, "# sample rate: %d S/s\n", frequency*sample);
	fprintf(fp, "# Decimation factor: 1\n");
	fprintf(fp, "# Content: iq\n");
	fprintf(fp, "# Sensors ID: ");
	for (i=0;i<6;i++){
		if(sensor_connected[i]){
			fprintf(fp, "%d, ", i);
		}
	}
	fprintf(fp, "\n# Sensors FOP: 178860,178406 Hz\n");
	fprintf(fp, "# Sensors NB Samples:"); 
	for (i=0;i<6;i++){
		if(sensor_connected[i]){
			fprintf(fp, "%d, ", sample);
		}
	}
	fprintf(fp, "\n# Sensors NB First samples skipped: 0,0\n");	
	fprintf(fp, "# time [s],tx_id,rx_id, range [cm],intensity [a.u.], target_detected, ");
	for(i=0;i<sample;i++){
		fprintf(fp, "i_data_%d, ", i);
	}
	for(i=0;i<sample;i++){
		fprintf(fp, "q_data_%d, ", i);
	}
	fprintf(fp,"\n");
}
int main(int argc, char *argv[]){
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
	char sensor_connection[6];	
    int nfds, num_open_fds;
    struct pollfd pfds[1];
    int dur, sample, freq;
    char *log_file;
    
    buffer = (char*)orig_buffer;
		
	printf("tdk load chirp firmware, version 1.2\n");
	// get absolute IIO path & build MPU's sysfs paths
	if (process_sysfs_request(sysfs_path) < 0) {
		printf("Cannot find %s sysfs path\n", CHIRP_NAME);
		exit(0);
	}

	printf("%s sysfs path: %s, dev path=%s\n", CHIRP_NAME, sysfs_path, dev_path);
		
    dur = 10;
    sample = 40;
    freq = 5;
    log_file = "/usr/chirp.csv";
	int c;
	
	opterr = 0;	
	while ((c = getopt (argc, argv, "d:s:f:l:")) != -1){
	switch (c)
	  {
	  case 'd':
		dur= atoi(optarg);
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
	  default:
		abort ();
	  }
	}
	printf("options11, log file=%s, frequency=%d, samples=%d, duration=%d seconds\n",  log_file, freq, sample, dur);
    
	index = 0;
	counter = freq*dur;
	
    
	for (i=0;i<6;i++) {
		sprintf(file_name, "%s/in_positionrelative%d_raw", sysfs_path, i+18);
		if((fp = fopen(file_name, "wt")) == NULL) {
			printf("error opening %s\n", file_name);
			exit(0);
		} else{
			printf("open %s OK, with %d\n", file_name, sample);
		}
		fprintf(fp, "%d", sample);
		fclose(fp);
	}
	
	check_sensor_connection();
		
	last_timestamp = 0;
	timestamp = 0;
	scan_bytes = 0;
	num_sensors = 0;
	for(i=0;i<6;i++){
		scan_bytes += sensor_connected[i]*32;
		num_sensors += sensor_connected[i];
	}
	index = 0;
	for(i=0;i<6;i++) {
		if(sensor_connected[i] == 1) {
			sensor_connection[index] = i;
			index++;
		}
	}
	if((log_fp=fopen(log_file, "wt"))==NULL){
		printf("error opening log file %s\n", log_file);
		exit(0);
	}
	print_header(sample, freq, log_fp);	
	//fclose(log_fp);
	//return 0;

	scan_bytes += 32;	
	
	printf("counter=%d\n", counter);
	
	// counter controls how many times it will run.
	sprintf(file_name, "%s/calibbias", sysfs_path);
	if((fp = fopen(file_name, "wt")) == NULL) {
		printf("error opening %s\n", file_name);
		exit(0);
	} else{
		printf("open %s OK\n", file_name);
	}
	fprintf(fp, "%d", counter);
	fclose(fp);	

	sprintf(file_name, "%s/sampling_frequency", sysfs_path);
	if((fp = fopen(file_name, "wt")) == NULL) {
		printf("error opening %s\n", file_name);
		exit(0);
	} else{
		printf("open %s OK\n", file_name);
	}
	fprintf(fp, "%d", freq);
	fclose(fp);	

	for(i=0;i<40;i++){
		buffer[i] = 0;
	}
	total_bytes = 0;
	switch_streaming(1);


	pfds[0].fd = open(dev_path, O_RDONLY);	
	pfds[0].events = (POLLIN | POLLRDNORM);
	last_timestamp = 0;
	
	ready=1;
	while(ready == 1) {	

		char *tmp;
		short value;
		char *ptr;
		int dev_num;
				
		pfds[0].revents = 0;
		//printf("before new polling counter=%d\n", counter);
		nfds = 1;
		ready = poll(pfds, nfds, 1000);
		//printf("pass poll 0x%x, ready=%d\n", pfds[0].revents, ready);
		if (ready == -1)
            printf("poll error\n");
		if (pfds[0].revents & (POLLIN | POLLRDNORM)) {

			target_bytes = scan_bytes;
			bytes = read(pfds[0].fd, buffer, target_bytes);
			total_bytes += bytes;
			tmp = (char *)&timestamp;
			memcpy(tmp, &buffer[scan_bytes - 8], 8);
			//amplitude 2 bytes + intensity data 2 bytes 224/8 = 28bytes(IQ)+mode(1 bytes)
			target_bytes = scan_bytes-32;
			i = 0;
			for(i=0;i<64;i++){
				//printf("%d, ", buffer[i]);
			}
			//printf("\n");
			if (last_timestamp == 0) {
				last_timestamp = timestamp;
			}
			
			if (last_timestamp != timestamp) {
				index -= 7;
				
				
				for(dev_num=0;dev_num<num_sensors;dev_num++) {
					fprintf(log_fp, "%f, ", last_timestamp/1000000000.0);
					//printf("%f, ", last_timestamp/1000000000.0);
				
					for(j=0;j<num_sensors;j++) {
						if(mode[j]==0x10){
							fprintf(log_fp, "%d, ", sensor_connection[j]);

						}
					}					
					
					for(j=0;j<num_sensors;j++) {
						if(mode[j]==0x20){
							fprintf(log_fp, "%d, ", sensor_connection[j]);
						}
					}					
					fprintf(log_fp, "%d, ", distance[dev_num]);
					fprintf(log_fp, "%d, ", amplitude[dev_num]);
					if(distance[dev_num]==0xFFFF){
						fprintf(log_fp, "%d, ", 0);
					}else {
						fprintf(log_fp, "%d, ", 1);
					}


					for(i=0;i<sample;i++) {
						fprintf(log_fp, "%d, ", I[dev_num][i]);
						//printf("%d, ", I[dev_num][i]);
					}
					for(i=0;i<sample;i++) {
						fprintf(log_fp, "%d, ", Q[dev_num][i]);
						//printf("%d, ", Q[dev_num][i]);
					}
					fprintf(log_fp, "\n");
				
					last_timestamp = timestamp;
					index = 0;
				}
			}
			
			//printf("\nindex=%d\n", index);
			for(j=0;j<num_sensors;j++) {
				for(i=0;i<7;i++){
					value = buffer[i*4+1+j*28];
					value <<=8;
					value += buffer[i*4+j*28];
					I[j][i+index] = value;

					value = buffer[i*4+3+j*28];
					value <<=8;
					value += buffer[i*4+2+j*28];
					
					Q[j][i+index] = value;
					//printf("%d, %d", I[j][i], Q[j][i]);					
				}
			}
			//printf("\n");
			index += 7;
			
			ptr = buffer + 28*num_sensors;

			for(j=0;j<num_sensors;j++) {
				distance[j] = ptr[1+j*2];
				distance[j] <<= 8;
				distance[j] += ptr[j*2];
			}
			
			ptr += 2*num_sensors;
			
			for(j=0;j<num_sensors;j++) {
				amplitude[j] = ptr[1+j*2];
				amplitude[j] <<= 8;
				amplitude[j] += ptr[j*2];			
			}

			ptr += 2*num_sensors;
			
			for(j=0;j<num_sensors;j++) {
				mode[j] = ptr[j];
			}				
		}
	}
	switch_streaming(0);
	fclose(log_fp);
	
	return 0;	
}
