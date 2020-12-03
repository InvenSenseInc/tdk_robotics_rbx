#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "ch101.h"

#define CHIRP_NAME		"ch101"
#define IIO_DIR 		"/sys/bus/iio/devices/"
#define FIRMWARE_PATH		"/usr/share/tdk/"
#define MAX_SYSFS_NAME_LEN	(100)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

extern char ch101_gpr_version[CH101_FW_VERS_SIZE];	// version string
extern u8 ch101_gpr_fw[CH101_FW_SIZE];

static const char * fw_names[] = {
"v39.hex",
"v39_IQ_debug-02.hex",
"ch101_gpr_rxopt_v41b.bin",
"ch101_gpr_rxopt_v41b-IQ_Debug.bin",
"ch201_old.bin",
"ch201_gprmt_v10a.bin",
"ch201_gprmt_v10a-IQ_Debug.bin",
};

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
	return 0;
}

int inv_load_dmp(char *dmp_path, int dmp_version, const char *dmp_firmware_path)
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

	sprintf(firmware_file, "%s/misc_bin_dmp_firmware_vers", dmp_path);
	if ((fp = fopen(firmware_file, "wb")) == NULL) {
		printf("dmp sysfs file %s open fail\n", firmware_file);
		fclose(fp_read);
		return -1;
	}
	
	printf("file=%s\n", firmware_file);

	p = (const void *)fw_names[dmp_version];
	write_size = strlen(p);
	if(write_size > 30) {
		write_size = 30;
	}
	printf("stname=%s\n", p);
		
	bytesWritten = fwrite(p, 1, write_size, fp);
	if (bytesWritten != write_size) {
		printf(
			"bytes written (%d) don't match "
			"requested length (%d)\n",
			bytesWritten, write_size);
		result = -1;
	}

	fclose(fp);

	if (result == 0)
		printf("dmp firmware %s written to %s\n",
			fw_names[dmp_version], firmware_file);

	if (dmp_version >= fw_names_size) {
		printf("dmp_version %d out of range [0-%d]\n",
			dmp_version, fw_names_size);
		return -1;
	}
	

	sprintf(bin_file, "%s%s", dmp_firmware_path, fw_names[dmp_version]);

	if ((fp_read = fopen(bin_file, "rb")) == NULL) {
		printf("dmp firmware file %s open fail\n", bin_file);
		return -1;
	}

	fseek(fp_read, 0L, SEEK_END);
	len = ftell(fp_read);
	fseek(fp_read, 0L, SEEK_SET);
	sprintf(firmware_file, "%s/misc_bin_dmp_firmware", dmp_path);

	if ((fp = fopen(firmware_file, "wb")) == NULL) {
		printf("dmp sysfs file %s open fail\n", firmware_file);
		fclose(fp_read);
		return -1;
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
				"bytes written (%d) don't match "
				"requested length (%d)\n",
				bytesWritten, write_size);
			result = -1;
		}
		len -= write_size;
	}

	fclose(fp);
	fclose(fp_read);


	return result;
}


int main(int argc, char *argv[])
{
	int fw_id = 0;
	char sysfs_path[MAX_SYSFS_NAME_LEN] = {0};

	if (argc > 1)
		fw_id = atoi(argv[1]);
		
	printf("tdk load chirp firmware, version 1.2\n");
	// get absolute IIO path & build MPU's sysfs paths
	if (process_sysfs_request(sysfs_path) < 0) {
		printf("Cannot find %s sysfs path\n", CHIRP_NAME);
	}

	printf("%s sysfs path: %s\n", CHIRP_NAME, sysfs_path);

	if (inv_load_dmp(sysfs_path, fw_id, FIRMWARE_PATH) != 0) {
		printf("dmp firmware fail\n");
		return -1;
	}

	return 0;
};
