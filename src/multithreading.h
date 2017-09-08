// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "logger.h"

void turn_on_light( void );
void turn_off_light( void );
void wait_light_on( void );
void wait_light_off( void );
int is_light_turned_on( bool ignore );

void sig_handler(int signo);
void run_test_timer(int duration);
