// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void prepare_logging(bool verbose, bool save_console_log, char *log_file_name);
void PRINT_LOG(char *x, char *y);
void PRINT_INFO(char *y);
void PRINT_INFO_FREE(char *y);
void PRINT_ERR(char *y);
void PRINT_ERR_FREE(char *y);
void PRINT_DBG(char *y);
void PRINT_DBG_FREE(char *y);

#define ASPRINTF(...) { \
	int nc = asprintf(__VA_ARGS__); \
	if (nc < 0) \
		PRINT_ERR("error occurs in asprintf"); \
}
