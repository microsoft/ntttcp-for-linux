import re
import time
import json
import pytest
import subprocess
from datetime import datetime
from typing import Optional, Tuple
from xml.sax import make_parser
from xml.sax.handler import ContentHandler
import logger


class NtttcpOutput:

    DECIMAL_BASED_UNIT_K = 1000

    def __init__(self, receiver_out: str, sender_out: str) -> None:
        self.receiver_stdout = receiver_out
        self.sender_stdout = sender_out

    def get_throughput_Gbps(self) -> float:
        throughput_Gbps = 0
        # 13:38:16 INFO:   throughput     :20.68Gbps
        throughput_Gbps_pattern = re.compile(
            r"INFO:\s+throughput\s+:(\d+\.\d+)(\w+)"
        )
        matched = throughput_Gbps_pattern.findall(self.sender_stdout)
        if matched:
            if matched[0][1] == "Gbps":
                throughput_Gbps = float(matched[0][0])
            elif matched[0][1] == "Mbps":
                throughput_Gbps = float(matched[0][0]) / 1024
            elif matched[0][1] == "Kbps":
                throughput_Gbps = float(matched[0][0]) / 1024 / 1024
            elif matched[0][1] == "bps":
                throughput_Gbps = float(matched[0][0]) / 1024 / 1024 / 1024
        log = logger.Logger()
        log.write_info('Throughput: {0}Gbps'.format(throughput_Gbps))
        return throughput_Gbps

    def is_daemon_running(self) -> bool:
        daemon_pattern = re.compile(r"INFO: main: run this tool in the background")
        if daemon_pattern.findall(self.receiver_stdout):
            return True
        else:
            return False

    def is_multi_clients_mode(self) -> bool:
        # multiple clients:                yes
        multi_clients_pattern = re.compile(r"multiple clients:\s+yes")
        if multi_clients_pattern.findall(self.receiver_stdout):
            return True
        else:
            return False

    def get_multi_threads_info(self) -> Optional[int]:
        # 60 connections created in 1154 microseconds
        multi_connections_pattern = re.compile(r"(\d+) connections created in (\d+) microseconds")
        matched_multi_connections = multi_connections_pattern.findall(self.sender_stdout)
        if matched_multi_connections:
            return int(matched_multi_connections[0][0])
        else:
            print("Not matched the multi threads pattern, please double check")

    def get_running_time(self) -> Optional[Tuple[int, int, int]]:
        # 05:57:34 INFO: Network activity progressing...
        network_start_pattern = re.compile(r"(\d+\:\d+\:\d+) INFO: Network activity progressing...")
        network_start = network_start_pattern.findall(self.sender_stdout)
        # 05:57:38 INFO: Test warmup completed.
        warmup_completed_pattern = re.compile(r"(\d+\:\d+\:\d+) INFO: Test warmup completed.")
        warmup_completed = warmup_completed_pattern.findall(self.sender_stdout)
        if warmup_completed and network_start:
            warmup_time = (datetime.strptime(warmup_completed[0], '%H:%M:%S') - datetime.strptime(network_start[0], '%H:%M:%S')).seconds
        # INFO: test duration    :5 seconds
        duration_pattern = re.compile(r"INFO: test duration\s+:(\d+)")
        matched_duration_time = duration_pattern.findall(self.sender_stdout)
        # 05:57:44 INFO: Test cooldown is in progress...
        cooldown_start_pattern = re.compile(r"(\d+\:\d+\:\d+) INFO: Test cooldown is in progress...")
        cooldown_start = cooldown_start_pattern.findall(self.sender_stdout)
        # wait_light_off();
        # 05:57:48 INFO: Test cycle finished.
        test_cycle_pattern = re.compile(r"(\d+\:\d+\:\d+) INFO: Test cycle finished.")
        test_cycle = test_cycle_pattern.findall(self.sender_stdout)
        if cooldown_start and test_cycle:
            cooldown_time = (datetime.strptime(test_cycle[0], '%H:%M:%S') - datetime.strptime(cooldown_start[0], '%H:%M:%S')).seconds
        if warmup_time and matched_duration_time and cooldown_time:
            return warmup_time, int(matched_duration_time[0]), cooldown_time
        else:
            print("Can not match the running time")

    def is_epoll(self) -> bool:
        # *** use epoll()
        epoll_pattern = re.compile(r"\*\*\*\s+use epoll()")
        if epoll_pattern.findall(self.receiver_stdout):
            return True
        else:
            return False

    def get_buffer_size(self) -> Optional[int]:
        # receiver socket buffer (bytes):  1470
        buffer_size_pattern = re.compile(r"receiver socket buffer \(bytes\):\s+(\d+)")
        buffer_size = buffer_size_pattern.findall(self.receiver_stdout)
        if buffer_size:
            matched_buffer_size = buffer_size[0]
            return int(matched_buffer_size)
        else:
            print("Can not match the buffer_size")

    def parse_xml_file(self, filename) -> bool:
        """Determine the format of the xml file"""
        parser = make_parser()
        parser.setContentHandler(ContentHandler())
        parser.parse(filename)
        if parser.parse(filename):
            return True
        else:
            return False

    def is_throughput_found_in_log_file(self, cmd_typle, filename) -> Optional[bool]:
        filename_out = open(filename, "r")
        value_filename_out = filename_out.read()
        if cmd_typle == "-O mylog.log":
            throughput_in_logfile_pattern = re.compile(r"INFO:\s+throughput\s+:(\d+\.\d+)(\w+)")
            throughput_in_logfile = throughput_in_logfile_pattern.findall(value_filename_out)
            if subprocess.run(f"ls {filename}", shell=True, check=True) and throughput_in_logfile:
                return True
            else:
                return False
        elif cmd_typle == "-x myxml.xml":
            throughput_in_xmlfile_pattern = re.compile(r"<throughput metric=\"MB\/s\">(\d+)\.(\d+)")
            throughput_in_xmlfile = throughput_in_xmlfile_pattern.findall(value_filename_out)
            if subprocess.run(f"ls {filename}", shell=True, check=True) and throughput_in_xmlfile:
                try:
                    self.parse_xml_file(filename)
                    return True
                except Exception:
                    return False
        elif cmd_typle == "-j myjson.json":
            throughput_in_jsonfile_pattern = re.compile(r"\"throughputs\" : \[[\s\S]*\{[\s\S]*\"metric\" : \"MB\/s\"\,[\s\S]*\"value\" : \"(\d+\.\d+)\"")
            throughput_in_jsonfile = throughput_in_jsonfile_pattern.findall(value_filename_out)
            if subprocess.run(f"ls {filename}", shell=True, check=True) and throughput_in_jsonfile:
                return True
            else:
                return False

    def get_starting_port_number(self) -> Optional[int]:
        # DBG : New connection: local:48536 [socket:4] --> 127.0.0.1:10000
        starting_port_pattern = re.compile(rf"DBG : New connection: local:(\d+) \[socket:4\] --> 127.0.0.1:(\d+)")
        starting_port = starting_port_pattern.findall(self.sender_stdout)
        if starting_port:
            print("starting_port:", starting_port[0][1])
            return int(starting_port[0][1])
        else:
            print("Cannot get the port number.")

    def get_ports_numbers(self) -> Optional[int]:
        # 800 connections tested
        mapping_option_value_pattern = re.compile(r"(\d+) connections tested")
        mapping_option_value = mapping_option_value_pattern.findall(self.sender_stdout)
        if mapping_option_value[0]:
            return int(mapping_option_value[0])
        else:
            return False

    def is_having_system_tcp_retransmit(self) -> bool:
        # INFO:   retrans_segments/sec   :2018.11
        system_tcp_retransmit_pattern = re.compile(r"retrans_segments/sec\s+:(\d+\.\d+)")
        system_tcp_retransmit = system_tcp_retransmit_pattern.findall(self.sender_stdout)
        if system_tcp_retransmit:
            return True
        else:
            return False

    def is_show_nic_packets(self) -> bool:
        # INFO:   tx_packets     :329598
        tx_packets_pattern = re.compile(r"\s+tx_packets\s:(\d+)")
        tx_packets = tx_packets_pattern.findall(self.sender_stdout)
        # INFO:   rx_packets     :329598
        rx_packets_pattern = re.compile(r"\s+rx_packets\s:(\d+)")
        rx_packets = rx_packets_pattern.findall(self.sender_stdout)
        if tx_packets and rx_packets and int(tx_packets[0]) > 100000 and int(rx_packets[0]) > 100000:
            print("tx_packets:", tx_packets[0])
            print("rx_packets:", rx_packets[0])
            return True
        else:
            return False

    def is_show_dev_interrupts(self) -> bool:
        # INFO:   total          :15
        interrupts_pattern = re.compile(r"total\s+:(\d+)")
        interrupts = interrupts_pattern.findall(self.sender_stdout)
        if interrupts and int(interrupts[0]) > 0:
            print("interrupts:", interrupts[0])
            return True
        else:
            return False
