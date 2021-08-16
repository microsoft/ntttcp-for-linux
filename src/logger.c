// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "logger.h"

bool verbose_log = false;
char *console_log_filename = NULL;
bool skip_log_file_open_error = false;

void prepare_logging(bool verbose, bool save_console_log, char *log_file_name)
{
	if (verbose)
		verbose_log = true;

	if (save_console_log) {
		if (log_file_name) {
			console_log_filename = log_file_name;
		} else {
			PRINT_ERR("the log file name is not specified");
		}

		if (access(console_log_filename, F_OK) != -1) {
			PRINT_INFO("log file exists. try to remove it before writing logs");
			if (remove(console_log_filename) != 0)
				PRINT_ERR("removing the existing log file failed");
		}
	}
}

void PRINT_LOG(char *x, char *y)
{
	time_t rawtime;
	struct tm *timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 80, "%H:%M:%S", timeinfo);
	printf("%s %s: %s\n", buffer, x, y);
	fflush(stdout);

	if (!console_log_filename)
		return;

	FILE *console_log_file = fopen(console_log_filename, "a");
	if (console_log_file == NULL) {
		if (!skip_log_file_open_error) {
			skip_log_file_open_error = true;
			printf("%s %s: %s\n", buffer, "ERR ", "error when capturing console log into file (only report once)");
		}
	} else {
		fprintf(console_log_file, "%s %s: %s\n", buffer, x, y);
		fclose(console_log_file);
	}
}

void PRINT_LOG_FREE(char *x, char *y)
{
	PRINT_LOG(x, y);
	free(y);
}

void PRINT_INFO(char *y)
{
	PRINT_LOG("INFO", y);
}

void PRINT_INFO_FREE(char *y)
{
	PRINT_LOG_FREE("INFO", y);
}

void PRINT_ERR(char *y)
{
	PRINT_LOG("ERR ", y);
}

void PRINT_ERR_FREE(char *y)
{
	PRINT_LOG_FREE("ERR ", y);
}

void PRINT_DBG(char *y)
{
	if (verbose_log) {
		PRINT_LOG("DBG ", y);
	}
}

void PRINT_DBG_FREE(char *y)
{
	if (verbose_log) {
		PRINT_LOG_FREE("DBG ", y);
	} else {
		free(y);
	}
}