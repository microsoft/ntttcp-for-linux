// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "oscounter.h"
#include <sys/sysctl.h>
#include <sys/types.h>
#define CPUSTATES 5
void get_cpu_usage(struct cpu_usage *cu)
{
	struct timeval time;
	struct rusage usage;

	gettimeofday(&time, NULL);
	cu->time = time.tv_sec * 1000000.0 + time.tv_usec;
	cu->clock = clock();
	getrusage(RUSAGE_SELF, &usage);
	cu->user_time = usage.ru_utime.tv_sec * 1000000.0 + usage.ru_utime.tv_usec;
	cu->system_time = usage.ru_stime.tv_sec * 1000000.0 + usage.ru_stime.tv_usec;
}

void get_cpu_usage_from_proc_stat(struct cpu_usage_from_proc_stat *cups)
{
//	unsigned long long int user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
//	user = nice = system = idle = iowait = irq = softirq = steal = guest = guest_nice = 0;

//	FILE *file = fopen(PROC_FILE_STAT, "r");
//	if (file == NULL) {
//		PRINT_ERR("Cannot open /proc/stat");
//		return;
//	}

//	char buffer[256];
//	int cpus = -1;
//	do {
//		cpus++;
//		char *s = fgets(buffer, 255, file);
//		/* We should not reach to the file end, because we only read lines starting with 'cpu' */
//		if (s == NULL) {
//			PRINT_ERR("Error when reading /proc/stat");
//			fclose(file);
//			return;
//		}
//
//		/* Assume the first line is stat for all CPUs. */
//		if (cpus == 0) {
//			sscanf(buffer,
//			      "cpu  %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu",
//			      &user, &nice, &system, &idle, &iowait,
//			      &irq, &softirq, &steal, &guest, &guest_nice);
//		}
//	} while (buffer[0] == 'c' && buffer[1] == 'p' && buffer[2] == 'u');
//
//	fclose(file);
//
	size_t len, len1;
	uint64_t cp_time[CPUSTATES];
	uint64_t cpu_num[1];
	unsigned long long nice;
	len = sizeof(cp_time);
	if (sysctlbyname("kern.cp_time", cp_time, &len, NULL, 0) == -1) {
		perror("sysctl");
		exit(EXIT_FAILURE);
	}
	len1 = sizeof(cpu_num);
	if (sysctlbyname("hw.ncpu", cpu_num, &len, NULL, 0) == -1) {
		perror("sysctl1");
		exit(EXIT_FAILURE);
	}
	cups->nproc = cpu_num[0];
	cups->user_time = cp_time[0];
	nice = cp_time[1];
	cups->system_time = cp_time[2];
	cups->idle_time = cp_time[4];
	cups->softirq_time = cp_time[3];

	cups->total_time = cp_time[0] + cp_time[1] + cp_time[2] + cp_time[3] + cp_time[4];
}

bool is_str_number(char *str)
{
	bool is_number = false;
	while (*str) {
		if (!isdigit(*str)) {
			if (*str != 10 && *str != 13)
				is_number = false;
			break;
		} else {
			is_number = true;
			++str;
		}
	}

	return is_number;
}

uint64_t get_interrupts_from_proc_by_dev(char *dev_name)
{
	FILE *file = fopen(PROC_FILE_INTERRUPTS, "r");
	if (file == NULL) {
		PRINT_ERR("Cannot open /proc/interrupts");
		return 0;
	}
	if (!strcmp(dev_name, ""))
		return 0;

	/* the max number of chars in each line = 64 + 12 chars/cpu * 1024 cpus + 128 = 12,480 */
	char buffer[12480];
	char *line;
	char *value;
	uint64_t interrupts = 0;
	uint64_t total_interrupts = 0;
	while ((line = fgets(buffer, 12480, file)) != NULL) {
		if (strstr(line, dev_name) == NULL) {
			continue;
		}

		value = strtok(line, " ");
		while (value != NULL) {
			if (!is_str_number(value)) {
				/* go to next element ("value")  */
				value = strtok(NULL, " ");
				continue;
			}
			errno = 0;
			interrupts = strtoull(value, NULL, 10);
			total_interrupts += (errno == 0) ? interrupts : 0;
			/* go to next element ("value")  */
			value = strtok(NULL, " ");
		}
	}
	fclose(file);

	return total_interrupts;
}

uint64_t get_single_value_from_os_file(char *if_name, char *tx_or_rx)
{
	char *log = NULL;
	char buffer[16];
	char *line;
	uint64_t value = 0;
	char *filename;

	if (!strcmp(if_name, ""))
		return 0;

	ASPRINTF(&filename, SYS_CLASS_NIC_STAT_PKT, if_name, tx_or_rx);
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		ASPRINTF(&log, "Cannot open %s", filename);
		PRINT_ERR_FREE(log);
		goto cleanup2;
	}

	line = fgets(buffer, 16, file);
	if (line == NULL) {
		ASPRINTF(&log, "Empty file: %s", filename);
		PRINT_ERR_FREE(log);
		goto cleanup1;
	}

	if (!is_str_number(line)) {
		ASPRINTF(&log, "Cannot convert the content to a number: %s", filename);
		PRINT_ERR_FREE(log);
		goto cleanup1;
	}

	value = strtoull(line, NULL, 10);

cleanup1:
	fclose(file);

cleanup2:
	free(filename);

	return value;
}

uint64_t read_counter_from_proc(char *file_name, char *section, char *key)
{
	char *log;
	FILE *stream;
	char *line = NULL, *pch = NULL;
	size_t len = 0;
	ssize_t read;
	int key_found = 0;
	uint64_t ret = 0;

	stream = fopen(file_name, "r");
	if (!stream) {
		ASPRINTF(&log, "failed to open file: %s. errno = %d", file_name, errno);
		PRINT_ERR_FREE(log);
		return 0;
	}

	/*
	 * example:
	 *
	 * Tcp: ... OutSegs RetransSegs InErrs OutRsts InCsumErrors
	 * Tcp: ... 8584582 27 0 5 0
	 *
	 * the first line contains the key;
	 * if a key is found, then,
	 * read the corresponding cell as value from next line
	 */
	while ((read = getline(&line, &len, stream)) != -1) {
		/* key is found, then read the value here */
		if (key_found > 0) {
			pch = line;
			while ((pch = strtok(pch, " "))) {
				key_found--;
				if (key_found == 0) {
					ret = strtoull(pch, NULL, 10);
					goto found;
				}
				pch = NULL;
			}
		}
		/* try to locate the key */
		if (strcmp(section, line) < 0) {
			if (strstr(line, key) != NULL) {
				pch = line;
				while ((pch = strtok(pch, " "))) {
					key_found++;
					if (strcmp(pch, key) == 0)
						break;
					pch = NULL;
				}
			}
		}
	}

found:
	free(line);
	fclose(stream);

	return ret;
}

void get_tcp_retrans(struct tcp_retrans *tr)
{
	tr->retrans_segs		= read_counter_from_proc(PROC_FILE_SNMP,    TCP_SECTION, "RetransSegs");
	tr->tcp_lost_retransmit		= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPLostRetransmit");
	tr->tcp_syn_retrans		= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPSynRetrans");
	tr->tcp_fast_retrans		= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPFastRetrans");
	tr->tcp_forward_retrans		= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPForwardRetrans");
	tr->tcp_slowStart_retrans	= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPSlowStartRetrans");
	tr->tcp_retrans_fail		= read_counter_from_proc(PROC_FILE_NETSTAT, TCP_SECTION, "TCPRetransFail");
}

double read_value_from_proc(char *file_name, char *key)
{
	char *log;
	FILE *stream;
	char *line = NULL, *pch = NULL;
	size_t len = 0;
	ssize_t read;
	double speed = 0;

	stream = fopen(file_name, "r");
	if (!stream) {
		ASPRINTF(&log, "failed to open file: %s. errno = %d", file_name, errno);
		PRINT_ERR_FREE(log);
		return 0;
	}

	/*
	 * example:
	 * ...
	 * cpu MHz         : 2394.462
	 * ...
	 */
	while ((read = getline(&line, &len, stream)) != -1) {
		if (strstr(line, key) != NULL) {
			pch = strstr(line, ":") + 1;
			break;
		}
	}
	speed = pch ? strtod(pch, NULL) : 0;
	if (speed == 0)
		PRINT_ERR("Failed to read CPU speed from /proc/cpuinfo");

	free(line);
	fclose(stream);

	return speed;
}
