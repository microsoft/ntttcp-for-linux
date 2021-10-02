import time
import pytest
import subprocess
import ParseNtttcpResult
from typing import Optional, Tuple

class TestNtttcp:
    expected_throughput = 25  # need evaluate
    loopback_interface = "127.0.0.1"
    set_duration_time_sec = 5
    n_server_ports = 4
    n_threads_per_server_port = 5
    n_connections_per_thread = 3
    total_sender_connections = n_server_ports * n_threads_per_server_port * n_connections_per_thread

    # todo need add setup and teardown
    def run_test(self, receiver_cmd: str, sender_cmd: str) -> ParseNtttcpResult.NtttcpResult:
        print(f"Receiver command: {receiver_cmd}")
        print(f"Sender command: {sender_cmd}")
        # every receiver command is with "-D", so the receiver command can return
        # at once. Otherwise, we need to use "subprocess.Popen"
        with open("receiver_log.txt", "wb") as receiver_out:
            subprocess.run([receiver_cmd], shell=True, stdout=receiver_out)
        receiver_open = open("receiver_log.txt", "r")
        receiver_out = receiver_open.read()
        receiver_open.close()
        time.sleep(1)
        with open("sender_log.txt", "wb") as sender_out:
            subprocess.run([sender_cmd], shell=True, stdout=sender_out)
        sender_open = open("sender_log.txt", "r")
        sender_out = sender_open.read()
        sender_open.close()

        return ParseNtttcpResult.NtttcpResult(
                receiver_out,
                sender_out
                )

    def combine_command(
        self,
        common_option: Optional[str] = "",
        receiver_option: Optional[str] = "",
        sender_option: Optional[str] = "",
    ) -> Tuple[str, str]:
        receiver_cmd = f"ulimit -n 40960 && ./src/ntttcp -r{self.loopback_interface} -t {self.set_duration_time_sec} -Q -D"
        sender_cmd = f"ulimit -n 40960 && ./src/ntttcp -s{self.loopback_interface} -t {self.set_duration_time_sec} -Q"
        if common_option:
            receiver_cmd = f"{receiver_cmd} {common_option}"
            sender_cmd = f"{sender_cmd} {common_option}"
        if receiver_option:
            receiver_cmd = f"{receiver_cmd} {receiver_option}"
        if sender_option:
            sender_cmd = f"{sender_cmd} {sender_option}"
        return receiver_cmd, sender_cmd

    def setup(self):
        time.sleep(1)
        print("set 1 second before every test.")

    def teardown(self):
        subprocess.run("killall ntttcp", shell=True)
        print("kill all ntttcp process to clean up")

    def test_daemon(self) -> None:
        receiver_cmd, sender_cmd = self.combine_command()
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        assert parse_result.is_daemon_running() is True
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_multi_clients_mode(self) -> None:
        receiver_option = "-M -V"
        sender_option = "-L"
        receiver_cmd, sender_cmd = self.combine_command(
            receiver_option=receiver_option, sender_option=sender_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        assert parse_result.is_multi_clients_mode() is True
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_multi_port_threads(self) -> None:
        common_option_tcp_and_udp = (f"-P {self.n_server_ports}", f"-P {self.n_server_ports} -u")
        sender_option = f"-n {self.n_threads_per_server_port} -l {self.n_connections_per_thread}"
        for common_option in common_option_tcp_and_udp:
            receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option, sender_option=sender_option
            )
            result = self.run_test(receiver_cmd, sender_cmd)
            parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
            assert parse_result.get_multi_threads_info() == self.total_sender_connections
            throughput = parse_result.get_throughput_Gbps()
            assert throughput >= self.expected_throughput

    def test_running_with_warmup_cooldowm_time(self) -> None:
        set_warmup_time = 3
        set_cooldown_time = 4
        common_option = f"-W {set_warmup_time} -C {set_cooldown_time}"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        (
            actual_warmup_time,
            actual_duration_time,
            actual_cooldown_time
        ) = parse_result.get_running_time()
        assert actual_warmup_time in range(set_warmup_time - 1, set_warmup_time + 1)
        assert actual_cooldown_time in range(set_cooldown_time - 1, set_cooldown_time + 1)
        assert actual_duration_time == self.set_duration_time_sec
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_epoll(self) -> None:
        receiver_option = "-e -V"
        receiver_cmd, sender_cmd = self.combine_command(
                receiver_option=receiver_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        assert parse_result.is_epoll() is True
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_buffer_size(self) -> None:
        buffer_size = 1470
        common_options = (f"-b {buffer_size} -V", f"-b {buffer_size} -V -u")
        for common_option in common_options:
            receiver_cmd, sender_cmd = self.combine_command(
                    common_option=common_option
            )
            result = self.run_test(receiver_cmd, sender_cmd)
            parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
            (
                actual_buffer_size_tcp
            ) = parse_result.get_buffer_size()
            assert actual_buffer_size_tcp == buffer_size

    def test_bandwidth_limit(self) -> None:
        throughput_limit_gbps = 10
        common_option = f"-B {throughput_limit_gbps}G"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        throughput = parse_result.get_throughput_Gbps()
        assert int(throughput) in range(throughput_limit_gbps - 1, throughput_limit_gbps + 1)

    def test_output_files(self) -> None:
        filenames = {'mylog.log': '-O mylog.log', 'myxml.xml': '-x myxml.xml', 'myjson.json': '-j myjson.json'}
        for filename in filenames:
            sender_option = filenames[filename]
            receiver_cmd, sender_cmd = self.combine_command(
                sender_option=sender_option
            )
            result = self.run_test(receiver_cmd, sender_cmd)
            parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
            assert parse_result.is_throughput_found_in_log_file(filenames[filename], filename)

    def test_starting_port_number(self) -> None:
        starting_port = 10000
        common_option = f"-p {starting_port}"
        sender_option = "-V"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option,
                sender_option=sender_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        assert parse_result.get_starting_port_number() == starting_port
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_mapping_option(self) -> None:
        ports = 200
        defualt_threads = 4
        receiver_cmd = f"ulimit -n 10240 && ntttcp -D -r -m {ports},*,{self.loopback_interface} -D -t 5"
        sender_cmd = f"ulimit -n 10240 && ntttcp -s{self.loopback_interface} -P {ports} -t 5"
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        assert parse_result.get_ports_numbers() == ports * defualt_threads
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_system_tcp_retransmit(self) -> None:
        common_option = "--show-tcp-retrans"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        assert parse_result.is_having_system_tcp_retransmit() is True
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_show_nic_packets(self) -> None:
        common_option = "--show-nic-packets lo"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option,
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        assert parse_result.is_show_nic_packets() is True
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_show_dev_interrupts(self, ) -> None:
        common_option = "--show-dev-interrupts Hypervisor callback interrupts"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        assert parse_result.is_show_dev_interrupts() is True
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_fq_rate_limit(self) -> None:
        throughput_limit_gbps = 10
        common_option = f"--fq-rate-limit {throughput_limit_gbps}G"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ParseNtttcpResult.NtttcpResult(result.receiver_stdout, result.sender_stdout)
        throughput = parse_result.get_throughput_Gbps()
        assert int(throughput) in range(throughput_limit_gbps - 1, throughput_limit_gbps + 1)

if __name__ == "__main__":
    pytest.main()
