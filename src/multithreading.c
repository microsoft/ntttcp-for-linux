// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "multithreading.h"

/************************************************************/
//		ntttcp multiple threads synch helper
/************************************************************/
static int run_light = 0;
static pthread_mutex_t light_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wait_light = PTHREAD_COND_INITIALIZER;

void turn_on_light( void )
{
	pthread_mutex_lock( &light_mutex );
	run_light = 1;
	pthread_cond_broadcast( &wait_light );
	pthread_mutex_unlock( &light_mutex );
}

void turn_off_light( void )
{
	pthread_mutex_lock( &light_mutex );
	run_light = 0;
	pthread_cond_broadcast( &wait_light );
	pthread_mutex_unlock( &light_mutex );
}

void wait_light_on( void )
{
	pthread_mutex_lock( &light_mutex );
	while (run_light == 0)
		pthread_cond_wait( &wait_light, &light_mutex );
	pthread_mutex_unlock( &light_mutex );
}

void wait_light_off( void )
{
	pthread_mutex_lock( &light_mutex );
	while (run_light != 0)
		pthread_cond_wait( &wait_light, &light_mutex );
	pthread_mutex_unlock( &light_mutex );
}

int is_light_turned_on( bool ignore )
{
	int temp;

	if (ignore)
		return true;

	pthread_mutex_lock( &light_mutex );
	temp = run_light;
	pthread_mutex_unlock( &light_mutex );

	return temp;
}

/************************************************************/
//		ntttcp timer and signal handle
/************************************************************/
void sig_handler(int signo)
{
	//Ctrl+C
	if (signo == SIGINT) {
		PRINT_INFO("Interrupted by Ctrl+C");

		if (is_light_turned_on(false))
			turn_off_light();
		else
			exit (1);
	}
}

void timer_fired()
{
	turn_off_light();
}

void run_test_timer(int duration)
{
	struct itimerval it_val;

	it_val.it_value.tv_sec = duration;
	it_val.it_value.tv_usec = 0;
	it_val.it_interval.tv_sec = 0;
	it_val.it_interval.tv_usec = 0;

	if (signal(SIGALRM, timer_fired) == SIG_ERR) {
		PRINT_ERR("unable to set test timer: signal SIGALRM failed");
		exit(1);
	}
	if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
		PRINT_ERR("unable to set test timer: setitimer ITIMER_REAL failed");
		exit(1);
	}
}
