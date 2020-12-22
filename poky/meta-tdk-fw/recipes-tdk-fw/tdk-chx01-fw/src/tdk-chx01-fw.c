// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 InvenSense, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "ch101.h"

#define CHIRP_NAME "ch101"
#define IIO_DIR  "/sys/bus/iio/devices/"
#define FIRMWARE_PATH	"/usr/share/tdk/"
#define MAX_SYSFS_NAME_LEN	(100)

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
	return 0;
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

static void print_help(void)
{
	printf("Usage:\n");
	printf("-h: print this help\n");
	printf("-n x: load pre-defined firmware\n");
	printf("    0: v39.hex, CH101 firmware\n");
	printf("    1: v39_IQ_debug-02.hex, canned firmware for CH101\n");
	printf("    2: ch101_gpr_rxopt_v41b.bin, new version of CH101");
	printf("firmware\n");
	printf("    3: ch101_gpr_rxopt_v41b-IQ_Debug.bin, new version");
	printf(" of CH101 canned firmware\n");
	printf("    4: ch201_old.bin, CH201 firmware\n");
	printf("    5: ch201_gprmt_v10a.bin, CH201 newer version");
	printf(" firmware\n");
	printf("    6: ch201_gprmt_v10a-IQ_Debug.bin, canned version of");
	printf(" CH201 firmware\n");
	printf("-p x: load firmware with specified name.\n");
	printf("      The file name must start with \"ch101_XX\"");
	printf(" or \"ch201_XX\" to indicate it is for CH101 or CH201\n");
	printf("      The firmware file must be pushed");
	printf(" to \"/usr/share/tdk\" before it is loaded.\n");
}

int main(int argc, char *argv[])
{
	int fw_id;
	char sysfs_path[MAX_SYSFS_NAME_LEN] = {0};
	int c;

	printf("tdk load chirp firmware, version 1.3\n");

	fw_id = 0;
	firmware_path = NULL;
	c = getopt(argc, argv, "hp:n:");
	if (c != -1) {
		switch (c) {
		case 'h':
			print_help();
			return 0;
		case 'n':
			fw_id = atoi(optarg);
			break;
		case 'p':
			firmware_path = optarg;
			fw_id = 0xff;
			break;
		default:
			abort();
		}
	}

	// get absolute IIO path & build MPU's sysfs paths
	if (process_sysfs_request(sysfs_path) < 0)
		printf("Cannot find %s sysfs path\n", CHIRP_NAME);

	printf("%s sysfs path: %s\n", CHIRP_NAME, sysfs_path);

	if (inv_load_dmp(sysfs_path, fw_id, FIRMWARE_PATH) != 0) {
		printf("dmp firmware fail\n");
		return -EINVAL;
	}

	return 0;
};
