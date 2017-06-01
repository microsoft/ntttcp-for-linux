// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

//include time.h when calling localtime()
#define PRINT_LOG(x, y) { \
	time_t rawtime; \
	struct tm * timeinfo; \
	char buffer [80]; \
	time(&rawtime); \
	timeinfo = localtime(&rawtime); \
	strftime(buffer, 80, "%H:%M:%S",timeinfo); \
	printf("%s %s: %s\n", buffer, x, y); \
	fflush(stdout); \
}

#define PRINT_LOG_FREE(x, y) { \
	PRINT_LOG(x, y) \
	free(y); \
}

#define PRINT_INFO(y) { PRINT_LOG("INFO", y) }
#define PRINT_INFO_FREE(y) { PRINT_LOG_FREE("INFO", y) }

#define PRINT_ERR(y) { PRINT_LOG("ERR ", y) }
#define PRINT_ERR_FREE(y) { PRINT_LOG_FREE("ERR ", y) }

#define PRINT_DBG(y) { \
	if(verbose_log) { \
		PRINT_LOG("DBG ", y) \
	} \
}

#define PRINT_DBG_FREE(y) { \
	if(verbose_log) { \
		PRINT_LOG_FREE("DBG ", y) \
	} \
}

#define ASPRINTF(...) { \
	int nc = asprintf(__VA_ARGS__); \
	if (nc < 0) \
		PRINT_ERR("error occurs in asprintf"); \
}
