// ----------------------------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
// Author: Shihua (Simon) Xiao, sixiao@microsoft.com
// ----------------------------------------------------------------------------------

#include "util.h"

void enable_fq_rate_limit(struct ntttcp_stream_client *sc, int sockfd)
{
	unsigned int fqrate_bytes = sc->socket_fq_rate_limit_bytes;
	if (fqrate_bytes > 0) {
		if (setsockopt(sockfd, SOL_SOCKET, SO_MAX_PACING_RATE, &fqrate_bytes, sizeof(fqrate_bytes)) < 0) {
			char *log = NULL;
			ASPRINTF(&log, "failed to set fq bit rate for socket[%d]", sockfd);
			PRINT_INFO_FREE(log);
		}
	}
}

void run_ntttcp_rtt_calculation_for_sender(struct ntttcp_test_endpoint *tep)
{
	uint i = 0;
	uint total_rtt = 0;
	uint num_average_rtt = 0;
	struct ntttcp_stream_client *sc;

	/* Calculate average RTT across all connections */
	for (i = 0; i < tep->total_threads; i++) {
		sc = tep->client_streams[i];
		/* all sender threads are data threads; no sync thread on sender side */

		if (sc->average_rtt != (uint)-1) {
			total_rtt += sc->average_rtt;
			num_average_rtt++;
		}
	}

	if (num_average_rtt > 0)
		tep->results->average_rtt = total_rtt / num_average_rtt;
}

double get_time_diff(struct timeval *t1, struct timeval *t2)
{
	return fabs((t1->tv_sec + (t1->tv_usec / 1000000.0)) - (t2->tv_sec + (t2->tv_usec / 1000000.0)));
}

double unit_atod(const char *s, int unit_k)
{
	double n;
	char suffix = '\0';

	sscanf(s, "%lf%c", &n, &suffix);
	switch (suffix) {
	case 'g':
	case 'G':
		n *= (unit_k * unit_k * unit_k);
		break;
	case 'm':
	case 'M':
		n *= (unit_k * unit_k);
		break;
	case 'k':
	case 'K':
		n *= unit_k;
		break;
	default:
		break;
	}
	return n;
}

const char *unit_bps[] = {
	"bps",
	"Kbps",
	"Mbps",
	"Gbps"
};

int process_test_results(struct ntttcp_test_endpoint *tep)
{
	struct ntttcp_test_endpoint_results *tepr = tep->results;
	unsigned int i;
	double cpu_speed_mhz;
	double test_duration = tepr->actual_test_time;
	uint64_t total_bytes = tepr->total_bytes;
	long double cpu_ps_total_diff;

	if (test_duration == 0)
		return -1;

	/*
	 * calculate for per-thread counters, even for receiver's sync thread
	 * we will ignore that sync thread when printing in "print_test_results()"
	 */
	for (i = 0; i < tep->total_threads; i++) {
		if (tep->results->threads[i]->is_sync_thread == true)
			continue;

		tepr->threads[i]->KBps = tepr->threads[i]->total_bytes / tepr->threads[i]->actual_test_time / DECIMAL_BASED_UNIT_K;
		tepr->threads[i]->MBps = tepr->threads[i]->KBps / DECIMAL_BASED_UNIT_K;
		tepr->threads[i]->mbps = tepr->threads[i]->MBps * BYTE_TO_BITS;
	}

	/* calculate for overall counters */
	cpu_speed_mhz = read_value_from_proc(PROC_FILE_CPUINFO, CPU_SPEED_MHZ);
	tepr->cpu_speed_mhz = cpu_speed_mhz;

	tepr->retrans_segments_per_sec = (tepr->final_tcp_retrans->retrans_segs - tepr->init_tcp_retrans->retrans_segs) / test_duration;
	tepr->tcp_lost_retransmit_per_sec = (tepr->final_tcp_retrans->tcp_lost_retransmit - tepr->init_tcp_retrans->tcp_lost_retransmit) / test_duration;
	tepr->tcp_syn_retrans_per_sec = (tepr->final_tcp_retrans->tcp_syn_retrans - tepr->init_tcp_retrans->tcp_syn_retrans) / test_duration;
	tepr->tcp_fast_retrans_per_sec = (tepr->final_tcp_retrans->tcp_fast_retrans - tepr->init_tcp_retrans->tcp_fast_retrans) / test_duration;
	tepr->tcp_forward_retrans_per_sec = (tepr->final_tcp_retrans->tcp_forward_retrans - tepr->init_tcp_retrans->tcp_forward_retrans) / test_duration;
	tepr->tcp_slowStart_retrans_per_sec = (tepr->final_tcp_retrans->tcp_slowStart_retrans - tepr->init_tcp_retrans->tcp_slowStart_retrans) / test_duration;
	tepr->tcp_retrans_fail_per_sec = (tepr->final_tcp_retrans->tcp_retrans_fail - tepr->init_tcp_retrans->tcp_retrans_fail) / test_duration;

	tepr->packets_sent = tepr->final_tx_packets - tepr->init_tx_packets;
	tepr->packets_received = tepr->final_rx_packets - tepr->init_rx_packets;
	tepr->total_interrupts = tepr->final_interrupts - tepr->init_interrupts;
	tepr->packets_per_interrupt = tepr->total_interrupts == 0 ? 0 : (tepr->packets_sent + tepr->packets_received) / (double)tepr->total_interrupts;

	cpu_ps_total_diff = tepr->final_cpu_ps->total_time - tepr->init_cpu_ps->total_time;
	tepr->cpu_ps_user_usage = (tepr->final_cpu_ps->user_time - tepr->init_cpu_ps->user_time) / cpu_ps_total_diff;
	tepr->cpu_ps_system_usage = (tepr->final_cpu_ps->system_time - tepr->init_cpu_ps->system_time) / cpu_ps_total_diff;
	tepr->cpu_ps_idle_usage = (tepr->final_cpu_ps->idle_time - tepr->init_cpu_ps->idle_time) / cpu_ps_total_diff;
	tepr->cpu_ps_softirq_usage = (tepr->final_cpu_ps->softirq_time - tepr->init_cpu_ps->softirq_time) / cpu_ps_total_diff;

	/* calculate for counters for xml log (compatible with Windows ntttcp.exe) */
	tepr->throughput_Bps  = (double)total_bytes / test_duration;
	tepr->total_bytes_MB  = (double)total_bytes / DECIMAL_BASED_UNIT_M;
	tepr->throughput_MBps = tepr->total_bytes_MB / test_duration;
	tepr->throughput_mbps = tepr->throughput_MBps * BYTE_TO_BITS;
	tepr->cycles_per_byte = total_bytes == 0 ? 0 :
				cpu_speed_mhz * 1000 * 1000 * test_duration * (tepr->final_cpu_ps->nproc) * (1 - tepr->cpu_ps_idle_usage) / total_bytes;
	tepr->packets_retransmitted = tepr->final_tcp_retrans->retrans_segs - tepr->init_tcp_retrans->retrans_segs;
	tepr->cpu_busy_percent = ((tepr->final_cpu_usage->clock - tepr->init_cpu_usage->clock) * 1000000.0 / CLOCKS_PER_SEC)
				 / (tepr->final_cpu_usage->time - tepr->init_cpu_usage->time);
	tepr->errors = 0;

	/* calculate TCP RTT */
	if (tep->endpoint_role == ROLE_SENDER && tep->test->protocol == TCP)
		run_ntttcp_rtt_calculation_for_sender(tep);

	return 0;
}

void print_test_results(struct ntttcp_test_endpoint *tep)
{
	struct ntttcp_test_endpoint_results *tepr = tep->results;
	uint64_t total_bytes = tepr->total_bytes;
	uint total_conns_created = 0;
	double test_duration = tepr->actual_test_time;

	unsigned int i;
	char *log = NULL, *log_tmp = NULL;

	if (test_duration == 0)
		return;

	if (tep->test->verbose) {
		PRINT_INFO("\tThread\tTime(s)\tThroughput");
		PRINT_INFO("\t======\t=======\t==========");
		for (i = 0; i < tep->total_threads; i++) {
			if (tep->results->threads[i]->is_sync_thread == true)
				continue;

			log_tmp = format_throughput(tepr->threads[i]->total_bytes,
						    tepr->threads[i]->actual_test_time);
			ASPRINTF(&log, "\t%d\t %.2f\t %s", i, tepr->threads[i]->actual_test_time, log_tmp);
			free(log_tmp);
			PRINT_INFO_FREE(log);
		}
	}

	/* only sender/client report the total connections established */
	if (tep->test->client_role == true) {
		for (i = 0; i < tep->total_threads; i++)
			total_conns_created += tep->client_streams[i]->num_conns_created;
		ASPRINTF(&log, "%d connections tested", total_conns_created);
		PRINT_INFO_FREE(log);
	}

	PRINT_INFO("#####  Totals:  #####");
	ASPRINTF(&log, "test duration\t:%.2f seconds", test_duration);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "total bytes\t:%" PRIu64, total_bytes);
	PRINT_INFO_FREE(log);

	log_tmp = format_throughput(total_bytes, test_duration);
	ASPRINTF(&log, "\t throughput\t:%s", log_tmp);
	free(log_tmp);
	PRINT_INFO_FREE(log);
	/* only show RetransSegs for TCP traffic */
	if (tepr->endpoint->test->protocol == TCP) {
		ASPRINTF(&log, "\t retrans segs\t:%"PRIu64, tepr->packets_retransmitted);
		PRINT_INFO_FREE(log);
	}

	if (tep->test->show_tcp_retransmit) {
		PRINT_INFO("tcp retransmit:");
		ASPRINTF(&log, "\t retrans_segments/sec\t:%.2f", tepr->retrans_segments_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t lost_retrans/sec\t:%.2f", tepr->tcp_lost_retransmit_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t syn_retrans/sec\t:%.2f", tepr->tcp_syn_retrans_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t fast_retrans/sec\t:%.2f", tepr->tcp_fast_retrans_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t forward_retrans/sec\t:%.2f", tepr->tcp_forward_retrans_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t slowStart_retrans/sec\t:%.2f", tepr->tcp_slowStart_retrans_per_sec);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t retrans_fail/sec\t:%.2f", tepr->tcp_retrans_fail_per_sec);
		PRINT_INFO_FREE(log);
	}

	if (strcmp(tep->test->show_interface_packets, "")) {
		PRINT_INFO("total packets:");
		ASPRINTF(&log, "\t tx_packets\t:%"PRIu64, tepr->packets_sent);
		PRINT_INFO_FREE(log);
		ASPRINTF(&log, "\t rx_packets\t:%"PRIu64, tepr->packets_received);
		PRINT_INFO_FREE(log);
	}
	if (strcmp(tep->test->show_dev_interrupts, "")) {
		PRINT_INFO("interrupts:");
		ASPRINTF(&log, "\t total\t\t:%" PRIu64, tepr->total_interrupts);
		PRINT_INFO_FREE(log);
	}
	if (strcmp(tep->test->show_interface_packets, "") && strcmp(tep->test->show_dev_interrupts, "")) {
		ASPRINTF(&log, "\t pkts/interrupt\t:%.2f", tepr->packets_per_interrupt);
		PRINT_INFO_FREE(log);
	}

	if (tepr->final_cpu_ps->nproc == tepr->init_cpu_ps->nproc) {
		ASPRINTF(&log, "cpu cores\t:%d", tepr->final_cpu_ps->nproc);
		PRINT_INFO_FREE(log);
	} else {
		ASPRINTF(&log,
			"number of CPUs does not match: initial: %d; final: %d",
			tepr->init_cpu_ps->nproc, tepr->final_cpu_ps->nproc);
		PRINT_ERR_FREE(log);
	}

	ASPRINTF(&log, "\t cpu speed\t:%.3fMHz", tepr->cpu_speed_mhz);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t user\t\t:%.2f%%", tepr->cpu_ps_user_usage * 100);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t system\t\t:%.2f%%", tepr->cpu_ps_system_usage * 100);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t idle\t\t:%.2f%%", tepr->cpu_ps_idle_usage * 100);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t softirq\t:%.2f%%", tepr->cpu_ps_softirq_usage * 100);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "\t cycles/byte\t:%.2f", tepr->cycles_per_byte);
	PRINT_INFO_FREE(log);
	ASPRINTF(&log, "cpu busy (all)\t:%.2f%%", tepr->cpu_busy_percent * 100);
	PRINT_INFO_FREE(log);

	if (tep->test->verbose) {
		if (tep->endpoint_role == ROLE_SENDER && tep->test->protocol == TCP) {
			ASPRINTF(&log, "tcpi rtt\t\t:%u us", tepr->average_rtt);
			PRINT_INFO_FREE(log);
		}
	}
	printf("---------------------------------------------------------\n");

	if (tep->test->save_xml_log)
		if (write_result_into_xml_file(tep) != 0)
			PRINT_ERR("Error writing log to xml file");
	if (tep->test->save_json_log)
		if (write_result_into_json_file(tep) != 0)
			PRINT_ERR("Error writing log to json file");
}

size_t execute_system_cmd_by_process(char *command, char *type, char **output)
{
	FILE *pfp;
	size_t count, len;

	pfp = popen(command, type);
	if (pfp == NULL) {
		PRINT_ERR("Error opening process to execute command");
		return 0;
	}

	count = getline(output, &len, pfp);

	fclose(pfp);
	return count;
}

unsigned int escape_char_for_xml(char *in, char *out)
{
	unsigned int count = 0;
	size_t pos_in = 0, pos_out = 0;

	for (pos_in = 0; in[pos_in]; pos_in++) {
		count++;
		switch (in[pos_in]) {
		case '>':
			memcpy(out + pos_out, "&gt;", 4);
			pos_out = pos_out + 4;
			break;
		case '<':
			memcpy(out + pos_out, "&lt;", 4);
			pos_out = pos_out + 4;
			break;
		case '&':
			memcpy(out + pos_out, "&amp;", 5);
			pos_out = pos_out + 5;
			break;
		case '\'':
			memcpy(out + pos_out, "&apos;", 6);
			pos_out = pos_out + 6;
			break;
		case '\"':
			memcpy(out + pos_out, "&quot;", 6);
			pos_out = pos_out + 6;
			break;
		case '\n':
			break;
		default:
			count--;
			memcpy(out + pos_out, in + pos_in, 1);
			pos_out++;
		}
	}
	return count;
}

unsigned int escape_char_for_json(char *in, char *out)
{
	unsigned int count = 0;
	size_t pos_in = 0, pos_out = 0;

	for (pos_in = 0; in[pos_in]; pos_in++) {
		count++;
		switch (in[pos_in]) {
		case '\n':
			break;
		default:
			count--;
			memcpy(out + pos_out, in + pos_in, 1);
			pos_out++;
		}
	}
	return count;
}

int write_result_into_xml_file(struct ntttcp_test_endpoint *tep)
{
	struct ntttcp_test *test = tep->test;
	struct ntttcp_test_endpoint_results *tepr = tep->results;
	char hostname[256];
	char *os_info = NULL;
	char os_info_escaped[2048];
	size_t count = 0;
	unsigned int i;
	int total_conns_created = 0;

	memset(hostname, '\0', sizeof(char) * 256);
	memset(os_info_escaped, '\0', sizeof(char) * 2048);

	FILE *logfile = fopen(test->xml_log_filename, "w");
	if (logfile == NULL) {
		PRINT_ERR("Error opening file to write log");
		return -1;
	}

	gethostname(hostname, 256);
	fprintf(logfile, "<ntttcp%s computername=\"%s\" version=\"5.33-linux\">\n", tep->endpoint_role == ROLE_RECEIVER ? "r" : "s", hostname);
	fprintf(logfile, "	<parameters>\n");
	fprintf(logfile, "		<send_socket_buff>%lu</send_socket_buff>\n", test->send_buf_size);
	fprintf(logfile, "		<recv_socket_buff>%lu</recv_socket_buff>\n", test->recv_buf_size);
	fprintf(logfile, "		<port>%d</port>\n", test->server_base_port);
	fprintf(logfile, "		<sync_port>%d</sync_port>\n", test->server_base_port - 1);
	fprintf(logfile, "		<no_sync>%s</no_sync>\n", test->no_synch == 0 ? "False" : "True");
	fprintf(logfile, "		<wait_timeout_milliseconds>%d</wait_timeout_milliseconds>\n", 0);
	fprintf(logfile, "		<async>%s</async>\n", "False");
	fprintf(logfile, "		<verbose>%s</verbose>\n", test->verbose ? "True" : "False");
	fprintf(logfile, "		<wsa>%s</wsa>\n", "False");
	fprintf(logfile, "		<use_ipv6>%s</use_ipv6>\n", test->domain == AF_INET6 ? "True" : "False");
	fprintf(logfile, "		<udp>%s</udp>\n", test->protocol == UDP ? "True" : "False");
	fprintf(logfile, "		<verify_data>%s</verify_data>\n", "False");
	fprintf(logfile, "		<wait_all>%s</wait_all>\n", "False");
	fprintf(logfile, "		<run_time>%d</run_time>\n", test->duration);
	fprintf(logfile, "		<warmup_time>%d</warmup_time>\n", test->warmup);
	fprintf(logfile, "		<cooldown_time>%d</cooldown_time>\n", test->cooldown);
	fprintf(logfile, "		<dash_n_timeout>%d</dash_n_timeout>\n", 0);
	fprintf(logfile, "		<bind_sender>%s</bind_sender>\n", "False");
	fprintf(logfile, "		<sender_name>%s</sender_name>\n", "NA");
	fprintf(logfile, "		<max_active_threads>%d</max_active_threads>\n", 0);
	fprintf(logfile, "		<tp>%s</tp>\n", "False");
	fprintf(logfile, "		<no_stdio_buffer>%s</no_stdio_buffer>\n", "False");
	fprintf(logfile, "		<throughput_Bpms>%d</throughput_Bpms>\n", 0);
	fprintf(logfile, "		<cpu_burn>%d</cpu_burn>\n", 0);
	fprintf(logfile, "		<latency_measurement>%s</latency_measurement>\n", "False");
	fprintf(logfile, "		<use_io_compl_ports>%s</use_io_compl_ports>\n", "NA");
	fprintf(logfile, "		<cpu_from_idle_flag>%s</cpu_from_idle_flag>\n", "False");
	fprintf(logfile, "		<get_estats>%s</get_estats>\n", "False");
	fprintf(logfile, "		<qos_flag>%s</qos_flag>\n", "False");
	fprintf(logfile, "		<jitter_measurement>%s</jitter_measurement>\n", "False");
	fprintf(logfile, "		<packet_spacing>%d</packet_spacing>\n", 0);
	fprintf(logfile, "	</parameters>\n");

	if (test->verbose) {
		for (i = 0; i < tep->total_threads; i++) {
			if (tep->results->threads[i]->is_sync_thread == true)
				continue;

			fprintf(logfile, "	<thread index=\"%i\">\n", i);
			fprintf(logfile, "		<realtime metric=\"s\">%.3f</realtime>\n", tepr->threads[i]->actual_test_time);
			fprintf(logfile, "		<throughput metric=\"KB/s\">%.3f</throughput>\n", tepr->threads[i]->KBps);
			fprintf(logfile, "		<throughput metric=\"MB/s\">%.3f</throughput>\n", tepr->threads[i]->MBps);
			fprintf(logfile, "		<throughput metric=\"mbps\">%.3f</throughput>\n", tepr->threads[i]->mbps);
			fprintf(logfile, "		<avg_bytes_per_compl metric=\"B\">%.3f</avg_bytes_per_compl>\n", 0.000);
			fprintf(logfile, "	</thread>\n");
		}
	}

	if (tep->test->client_role == true) {
		for (i = 0; i < tep->total_threads; i++)
			total_conns_created += tep->client_streams[i]->num_conns_created;
		fprintf(logfile, "	<conns_tested>%d</conns_tested>\n", total_conns_created);
	}

	fprintf(logfile, "	<total_bytes metric=\"MB\">%.6f</total_bytes>\n", tepr->total_bytes_MB);
	fprintf(logfile, "	<realtime metric=\"s\">%.6f</realtime>\n", tepr->actual_test_time);
	fprintf(logfile, "	<avg_bytes_per_compl metric=\"B\">%.3f</avg_bytes_per_compl>\n", 0.000);
	fprintf(logfile, "	<threads_avg_bytes_per_compl metric=\"B\">%.3f</threads_avg_bytes_per_compl>\n", 0.000);
	fprintf(logfile, "	<avg_frame_size metric=\"B\">%.3f</avg_frame_size>\n", 0.000);
	fprintf(logfile, "	<throughput metric=\"MB/s\">%.3f</throughput>\n", tepr->throughput_MBps);
	fprintf(logfile, "	<throughput metric=\"mbps\">%.3f</throughput>\n", tepr->throughput_mbps);
	fprintf(logfile, "	<throughput metric=\"Bps\">%.3f</throughput>\n", tepr->throughput_Bps);
	fprintf(logfile, "	<total_buffers>%.3f</total_buffers>\n", 0.000);
	fprintf(logfile, "	<throughput metric=\"buffers/s\">%.3f</throughput>\n", 0.000);
	fprintf(logfile, "	<avg_packets_per_interrupt metric=\"packets/interrupt\">%.3f</avg_packets_per_interrupt>\n", tepr->packets_per_interrupt);
	fprintf(logfile, "	<interrupts metric=\"count/sec\">%.3f</interrupts>\n", 0.000);
	fprintf(logfile, "	<dpcs metric=\"count/sec\">%.3f</dpcs>\n", 0.000);
	fprintf(logfile, "	<avg_packets_per_dpc metric=\"packets/dpc\">%.3f</avg_packets_per_dpc>\n", 0.000);
	fprintf(logfile, "	<packets_sent>%" PRIu64 "</packets_sent>\n", tepr->packets_sent);
	fprintf(logfile, "	<packets_received>%" PRIu64 "</packets_received>\n", tepr->packets_received);
	fprintf(logfile, "	<packets_retransmitted>%" PRIu64 "</packets_retransmitted>\n", tepr->packets_retransmitted);
	fprintf(logfile, "	<errors>%d</errors>\n", tepr->errors);
	fprintf(logfile, "	<bufferCount>%u</bufferCount>\n", 0);
	fprintf(logfile, "	<bufferLen>%u</bufferLen>\n", 0);
	
	if (tepr->final_cpu_ps->nproc == tepr->init_cpu_ps->nproc){
		fprintf(logfile, "	<cpu_cores metric=\"cpu cores\">%d</cpu_cores>\n", tepr->final_cpu_ps->nproc);
	} else {
		fprintf(logfile, "	<cpu_cores>number of CPUs does not match: initial: %d; final: %d</cpu_cores>\n",
		tepr->init_cpu_ps->nproc, tepr->final_cpu_ps->nproc);
	}
	fprintf(logfile, "	<cpu_speed metric=\"MHz\">%.3f</cpu_speed>\n", tepr->cpu_speed_mhz);
	fprintf(logfile, "	<user metric=\"%%\">%.2f</user>\n", tepr->cpu_ps_user_usage * 100);
	fprintf(logfile, "	<system metric=\"%%\">%.2f</system>\n", tepr->cpu_ps_system_usage * 100);
	fprintf(logfile, "	<idle metric=\"%%\">%.2f</idle>\n", tepr->cpu_ps_idle_usage * 100);
	fprintf(logfile, "	<softirq metric=\"%%\">%.2f</softirq>\n", tepr->cpu_ps_softirq_usage * 100);
	fprintf(logfile, "	<cycles_per_byte metric=\"cycles/byte\">%.2f</cycles_per_byte>\n", tepr->cycles_per_byte);
	fprintf(logfile, "	<cpu_busy_all metric=\"%%\">%.2f</cpu_busy_all>\n", tepr->cpu_busy_percent * 100);
	fprintf(logfile, "	<io>%u</io>\n", 0);
	if (test->verbose){
		if (tep->endpoint_role == ROLE_SENDER && test->protocol == TCP) {
			fprintf(logfile, "	<tcp_average_rtt metric=\"us\">%u</tcp_average_rtt>\n", tepr->average_rtt);
		}
	}
	
	count = execute_system_cmd_by_process("uname -a", "r", &os_info);
	if (os_info) {
		escape_char_for_xml(os_info, os_info_escaped);
		free(os_info);
	}
	fprintf(logfile, "	<os>%s</os>\n", count == 0 ? "Unknown" : os_info_escaped);
	fprintf(logfile, "</ntttcp%s>\n", tep->endpoint_role == ROLE_RECEIVER ? "r": "s");

	fclose(logfile);
	return 0;
}

int write_result_into_json_file(struct ntttcp_test_endpoint *tep)
{
	struct ntttcp_test *test = tep->test;
	struct ntttcp_test_endpoint_results *tepr = tep->results;
	char hostname[256];
	char *os_info = NULL;
	char os_info_escaped[2048];
	size_t count = 0;
	unsigned int i;

	memset(hostname, '\0', sizeof(char) * 256);
	memset(os_info_escaped, '\0', sizeof(char) * 2048);

	FILE *json_file = fopen(test->json_log_filename, "w");
	if (json_file == NULL) {
		PRINT_ERR("Error opening file to write log");
		return -1;
	}

	gethostname(hostname, 256);
	fprintf(json_file, "{\n");
	fprintf(json_file, "    \"ntttcp%s\" : {\n", tep->endpoint_role == ROLE_RECEIVER ? "r" : "s");
	fprintf(json_file, "        \"computername\" : \"%s\",\n", hostname);
	fprintf(json_file, "        \"version\" : \"5.33-linux\",\n");
	fprintf(json_file, "        \"parameters\" : {\n");
	fprintf(json_file, "            \"sendSocketBuff\" : \"%lu\",\n", test->send_buf_size);
	fprintf(json_file, "            \"recvSocketBuff\" : \"%lu\",\n", test->recv_buf_size);
	fprintf(json_file, "            \"port\" : \"%d\",\n", test->server_base_port);
	fprintf(json_file, "            \"syncPort\" : \"%d\",\n", test->server_base_port - 1);
	fprintf(json_file, "            \"noSync\" : \"%s\",\n", test->no_synch == 0 ? "False" : "True");
	fprintf(json_file, "            \"waitTimeoutMilliseconds\" : \"%d\",\n", 0);
	fprintf(json_file, "            \"async\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"verbose\" : \"%s\",\n", test->verbose ? "True" : "False");
	fprintf(json_file, "            \"wsa\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"useIpv6\" : \"%s\",\n", test->domain == AF_INET6 ? "True" : "False");
	fprintf(json_file, "            \"udp\" : \"%s\",\n", test->protocol == UDP ? "True" : "False");
	fprintf(json_file, "            \"verifyData\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"waitAll\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"runTime\" : \"%d\",\n", test->duration);
	fprintf(json_file, "            \"warmupTime\" : \"%d\",\n", test->warmup);
	fprintf(json_file, "            \"cooldownTime\" : \"%d\",\n", test->cooldown);
	fprintf(json_file, "            \"dashNTimeout\" : \"%d\",\n", 0);
	fprintf(json_file, "            \"bindSender\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"senderName\" : \"%s\",\n", "NA");
	fprintf(json_file, "            \"maxActiveThreads\" : \"%d\",\n", 0);
	fprintf(json_file, "            \"tp\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"noStdioBuffer\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"throughputBpms\" : \"%d\",\n", 0);
	fprintf(json_file, "            \"cpuBurn\" : \"%d\",\n", 0);
	fprintf(json_file, "            \"latencyMeasurement\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"useIOComplPorts\" : \"%s\",\n", "NA");
	fprintf(json_file, "            \"cpuFromIdleFlag\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"getEstats\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"qosFlag\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"jitterMeasurement\" : \"%s\",\n", "False");
	fprintf(json_file, "            \"packetSpacing\" : \"%d\"\n", 0);
	fprintf(json_file, "        },\n");

	if (test->verbose) {
		fprintf(json_file, "        \"threads\" : [\n");
		for (i = 0; i < tep->total_threads; i++) {
			if (tep->results->threads[i]->is_sync_thread == true) {
				/* Skip the sync thread */
				continue;
			} else if (i != 0) {
				/* Add separator between two thread array elements */
				fprintf(json_file, ",\n");
			}
			fprintf(json_file, "            {\n");
			fprintf(json_file, "                \"index\" : \"%d\",\n", i);
			fprintf(json_file, "                \"realTime\" : {\n");
			fprintf(json_file, "                    \"metric\" : \"s\",\n");
			fprintf(json_file, "                    \"value\" : \"%.3f\"\n", tepr->threads[i]->actual_test_time);
			fprintf(json_file, "                },\n");
			fprintf(json_file, "                \"throughputs\" : [\n");
			fprintf(json_file, "                    {\n");
			fprintf(json_file, "                        \"metric\" : \"KB/s\",\n");
			fprintf(json_file, "                        \"value\" : \"%.3f\"\n", tepr->threads[i]->KBps);
			fprintf(json_file, "                    },\n");
			fprintf(json_file, "                    {\n");
			fprintf(json_file, "                        \"metric\" : \"MB/s\",\n");
			fprintf(json_file, "                        \"value\" : \"%.3f bps\"\n", tepr->threads[i]->MBps);
			fprintf(json_file, "                    },\n");
			fprintf(json_file, "                    {\n");
			fprintf(json_file, "                        \"metric\" : \"mbps\",\n");
			fprintf(json_file, "                        \"value\" : \"%.3f\"\n", tepr->threads[i]->mbps);
			fprintf(json_file, "                    }\n");
			fprintf(json_file, "                ],\n");
			fprintf(json_file, "                \"avgBytesPerCompl\" : {\n");
			fprintf(json_file, "                    \"metric\" : \"B\",\n");
			fprintf(json_file, "                    \"value\" : \"%.3f\"\n", 0.000);
			fprintf(json_file, "                }\n");
			fprintf(json_file, "            }");
		}
		fprintf(json_file, "\n        ],\n");
	}

	fprintf(json_file, "        \"totalBytes\" : {\n");
	fprintf(json_file, "            \"metric\" : \"MB\",\n");
	fprintf(json_file, "            \"value\" : \"%.6f\"\n", tepr->total_bytes_MB);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"realTime\" : {\n");
	fprintf(json_file, "            \"metric\" : \"s\",\n");
	fprintf(json_file, "            \"value\" : \"%.6f\"\n", tepr->actual_test_time);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"avgBytesPerCompl\" : {\n");
	fprintf(json_file, "            \"metric\" : \"B\",\n");
	fprintf(json_file, "            \"value\" : \"%.3f\"\n", 0.000);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"threadsAvgBytesPerCompl\" : {\n");
	fprintf(json_file, "            \"metric\" : \"B\",\n");
	fprintf(json_file, "            \"value\" : \"%.3f\"\n", 0.000);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"avgFrameSize\" : {\n");
	fprintf(json_file, "            \"metric\" : \"B\",\n");
	fprintf(json_file, "            \"value\" : \"%.3f\"\n", 0.000);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"throughputs\" : [\n");
	fprintf(json_file, "            {\n");
	fprintf(json_file, "                \"metric\" : \"MB/s\",\n");
	fprintf(json_file, "                \"value\" : \"%.3f\"\n", tepr->throughput_MBps);
	fprintf(json_file, "            },\n");
	fprintf(json_file, "            {\n");
	fprintf(json_file, "                \"metric\" : \"mbps\",\n");
	fprintf(json_file, "                \"value\" : \"%.3f\"\n", tepr->throughput_mbps);
	fprintf(json_file, "            },\n");
	fprintf(json_file, "            {\n");
	fprintf(json_file, "                \"metric\" : \"Bps\",\n");
	fprintf(json_file, "                \"value\" : \"%.3f\"\n", tepr->throughput_Bps);
	fprintf(json_file, "            },\n");
	fprintf(json_file, "            {\n");
	fprintf(json_file, "                \"metric\" : \"buffer/s\",\n");
	fprintf(json_file, "                \"value\" : \"%.3f\"\n", 0.000);
	fprintf(json_file, "            }\n");
	fprintf(json_file, "        ],\n");
	fprintf(json_file, "        \"totalBuffers\" : \"%.3f\",\n", 0.000);
	fprintf(json_file, "        \"avgPacketsPerInterrupt\" : {\n");
	fprintf(json_file, "            \"metric\" : \"packets/interrupt\",\n");
	fprintf(json_file, "            \"value\" : \"%.3f\"\n", tepr->packets_per_interrupt);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"interrupt\" : {\n");
	fprintf(json_file, "            \"metric\" : \"count/sec\",\n");
	fprintf(json_file, "            \"value\" : \"%.3f\"\n", 0.000);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"dpcs\" : {\n");
	fprintf(json_file, "            \"metric\" : \"count/sec\",\n");
	fprintf(json_file, "            \"value\" : \"%.3f\"\n", 0.000);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"avgPacketsPerDpc\" : {\n");
	fprintf(json_file, "            \"metric\" : \"packets/dpc\",\n");
	fprintf(json_file, "            \"value\" : \"%.3f\"\n", 0.000);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"cycles\" : {\n");
	fprintf(json_file, "            \"metric\" : \"cycles/byte\",\n");
	fprintf(json_file, "            \"value\" : \"%.3f\"\n", tepr->cycles_per_byte);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"packetsSent\" : \"%"PRIu64"\",\n", tepr->packets_sent);
	fprintf(json_file, "        \"packetsReceived\" : \"%"PRIu64"\",\n", tepr->packets_received);
	fprintf(json_file, "        \"packets_retransmitted\" : \"%"PRIu64"\",\n", tepr->packets_retransmitted);
	fprintf(json_file, "        \"errors\" : \"%d\",\n", tepr->errors);
	fprintf(json_file, "        \"cpu\" : {\n");
	fprintf(json_file, "            \"metric\" : \"%%\",\n");
	fprintf(json_file, "            \"value\" : \"%.3f\"\n", tepr->cpu_busy_percent * 100);
	fprintf(json_file, "        },\n");
	fprintf(json_file, "        \"bufferCount\" : \"%u\",\n", 0);
	fprintf(json_file, "        \"bufferLen\" : \"%u\",\n", 0);
	fprintf(json_file, "        \"io\" : \"%u\",\n", 0);

	if(test->verbose){
		if (tep->endpoint_role == ROLE_SENDER && test->protocol == TCP) {
			fprintf(json_file, "        \"tcpAverageRtt metric\" : \"%u us\",\n", tepr->average_rtt);
		}
	}
	count = execute_system_cmd_by_process("uname -a", "r", &os_info);
	if (os_info) {
		escape_char_for_json(os_info, os_info_escaped);
		free(os_info);
	}
	fprintf(json_file, "        \"os\" : \"%s\"\n    }\n}\n", count == 0 ? "Unknown" : os_info_escaped);

	fclose(json_file);
	return 0;
}

char *format_throughput(uint64_t bytes_transferred, double test_duration)
{
	double tmp = 0;
	int unit_idx = 0;
	char *throughput;

	tmp = bytes_transferred * 8.0 / test_duration;
	while (tmp > DECIMAL_BASED_UNIT_K && unit_idx < 3) {
		tmp /= DECIMAL_BASED_UNIT_K;
		unit_idx++;
	}

	ASPRINTF(&throughput, "%.2f%s", tmp, unit_bps[unit_idx]);
	return throughput;
}

char *retrive_ip_address_str(struct sockaddr_storage *ss, char *ip_str, size_t maxlen)
{
	switch (ss->ss_family) {
	case AF_INET:
		inet_ntop(AF_INET, &(((struct sockaddr_in *)ss)->sin_addr), ip_str, maxlen);
		break;

	case AF_INET6:
		inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)ss)->sin6_addr), ip_str, maxlen);
		break;

	default:
		break;
	}
	return ip_str;
}

char *retrive_ip4_address_str(struct sockaddr_in *ss, char *ip_str, size_t maxlen)
{
	inet_ntop(AF_INET, &(ss->sin_addr), ip_str, maxlen);
	return ip_str;
}

char *retrive_ip6_address_str(struct sockaddr_in6 *ss, char *ip_str, size_t maxlen)
{
	inet_ntop(AF_INET6, &(ss->sin6_addr), ip_str, maxlen);
	return ip_str;
}

int set_socket_non_blocking(int fd)
{
	int flags, rtn;
	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;

	flags |= O_NONBLOCK;
	rtn = fcntl(fd, F_SETFL, flags);
	if (rtn == -1)
		return -1;

	return 0;
}

bool check_resource_limit(struct ntttcp_test *test)
{
	char *log;
	unsigned long soft_limit = 0;
	unsigned long hard_limit = 0;
	uint total_connections = 0;

	struct rlimit limitstruct;
	if (-1 == getrlimit(RLIMIT_NOFILE, &limitstruct))
		PRINT_ERR("Failed to load resource limits");

	soft_limit = (unsigned long)limitstruct.rlim_cur;
	hard_limit = (unsigned long)limitstruct.rlim_max;

	ASPRINTF(&log,
		"user limits for maximum number of open files: soft: %ld; hard: %ld",
		soft_limit, hard_limit);
	PRINT_DBG_FREE(log);

	if (test->client_role == true) {
		total_connections = test->server_ports * test->threads_per_server_port * test->conns_per_thread;
	} else {
		/*
		 * for receiver, just do a minimal check;
		 * because we don't know how many threads_per_server_port will be used by sender.
		 */
		total_connections = test->server_ports * 1;
	}

	if (total_connections > soft_limit) {
		ASPRINTF(&log,
			"soft limit is too small: limit is %ld; but total connections will be %d (or more on receiver)",
			 soft_limit, total_connections);
		PRINT_ERR_FREE(log);

		return false;
	} else {
		return true;
	}
}

bool check_is_ip_addr_valid_local(int ss_family, char *ip_to_check)
{
	struct ifaddrs *ifaddrp, *ifap;
	bool is_valid = false;
	char *ip_addr_str;
	size_t ip_addr_len;
	char *log;

	if ((ss_family == AF_INET) && strcmp(ip_to_check, "0.0.0.0") == 0)
		return true;

	if ((ss_family == AF_INET6) && strcmp(ip_to_check, "::") == 0)
		return true;

	ip_addr_len = (ss_family == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	if ((ip_addr_str = (char *)malloc(ip_addr_len)) == (char *)NULL) {
		PRINT_ERR("cannot allocate memory for ip address string");
		return false;
	}

	getifaddrs(&ifaddrp);

	for (ifap = ifaddrp; ifap; ifap = ifap->ifa_next) {
		if (ifap->ifa_addr && ifap->ifa_addr->sa_family == ss_family) {
			if (ss_family == AF_INET)
				inet_ntop(AF_INET, &(((struct sockaddr_in *)ifap->ifa_addr)->sin_addr), ip_addr_str, ip_addr_len);
			else
				inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)ifap->ifa_addr)->sin6_addr), ip_addr_str, ip_addr_len);

			ASPRINTF(&log, "Interface:[%s]\tAddress: %s", ifap->ifa_name, ip_addr_str);
			PRINT_DBG_FREE(log);

			if (strcmp(ip_to_check, ip_addr_str) == 0) {
				is_valid = true;
				/* do not break here; just want to loop all of the interfaces, for DBG log */
			}
		}
	}

	free(ip_addr_str);
	freeifaddrs(ifaddrp);
	return is_valid;
}
