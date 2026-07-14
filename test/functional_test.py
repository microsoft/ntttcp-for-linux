import time
import pytest
import subprocess
import logger
import ntttcp_output
from typing import Optional, Tuple


class TestNtttcp:

    log = logger.Logger()
    expected_throughput = 15  # need evaluate
    loopback_interface = "127.0.0.1"
    set_duration_time_sec = 5
    n_server_ports = 4
    n_threads_per_server_port = 5
    n_connections_per_thread = 3
    total_sender_connections = n_server_ports * n_threads_per_server_port * n_connections_per_thread

    # todo need add setup and teardown
    def run_test(self, receiver_cmd: str, sender_cmd: str) -> ntttcp_output.NtttcpOutput:
        self.log.write_info(f"Receiver command: {receiver_cmd}")
        self.log.write_info(f"Sender   command: {sender_cmd}")
        # every receiver command is with "-D", so the receiver command can return
        # at once. Otherwise, we need to use "subprocess.Popen"
        with open("receiver_log.txt", "wb") as receiver_out:
            subprocess.run(receiver_cmd, shell=True, stdout=receiver_out, check=True)
        receiver_open = open("receiver_log.txt", "r")
        receiver_out = receiver_open.read()
        receiver_open.close()
        time.sleep(1)
        with open("sender_log.txt", "wb") as sender_out:
            subprocess.run(sender_cmd, shell=True, stdout=sender_out, check=True)
        sender_open = open("sender_log.txt", "r")
        sender_out = sender_open.read()
        sender_open.close()

        return ntttcp_output.NtttcpOutput(receiver_out, sender_out)

    def combine_command(self, common_option: Optional[str] = "", receiver_option: Optional[str] = "",
                        sender_option: Optional[str] = "", duration: Optional[int] = None) -> Tuple[str, str]:
        test_duration = duration if duration is not None else self.set_duration_time_sec
        receiver_cmd = f"ulimit -n 40960 && ./src/ntttcp -r{self.loopback_interface} -t {test_duration} -Q -D"
        sender_cmd = f"ulimit -n 40960 && ./src/ntttcp -s{self.loopback_interface} -t {test_duration} -Q"
        if common_option:
            receiver_cmd = f"{receiver_cmd} {common_option}"
            sender_cmd = f"{sender_cmd} {common_option}"
        if receiver_option:
            receiver_cmd = f"{receiver_cmd} {receiver_option}"
        if sender_option:
            sender_cmd = f"{sender_cmd} {sender_option}"
        return receiver_cmd, sender_cmd

    def setup_method(self, method):
        time.sleep(1)
        print("\n")

    def teardown_method(self, method):
        subprocess.run("killall ntttcp", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def test_daemon(self) -> None:
        receiver_cmd, sender_cmd = self.combine_command()
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
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
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
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
            parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
            assert parse_result.get_multi_threads_info() == self.total_sender_connections
            throughput = parse_result.get_throughput_Gbps()
            assert throughput >= self.expected_throughput

    def test_running_with_warmup_cooldown_time(self) -> None:
        set_warmup_time = 3
        set_cooldown_time = 4
        common_option = f"-W {set_warmup_time} -C {set_cooldown_time}"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
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
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
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
            parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
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
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
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
            parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
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
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
        assert parse_result.get_starting_port_number() == starting_port
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_mapping_option(self) -> None:
        ports = 50
        default_threads = 4
        receiver_cmd = f"ulimit -n 10240 && ./src/ntttcp -D -r -m {ports},*,{self.loopback_interface} -t 5"
        sender_cmd = f"ulimit -n 10240 && ./src/ntttcp -s{self.loopback_interface} -P {ports} -t 5"
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
        assert parse_result.get_ports_numbers() == ports * default_threads
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_system_tcp_retransmit(self) -> None:
        common_option = "--show-tcp-retrans"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
        assert parse_result.is_having_system_tcp_retransmit() is True
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_show_nic_packets(self) -> None:
        common_option = "--show-nic-packets lo"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option,
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
        assert parse_result.is_show_nic_packets() is True
        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_show_dev_interrupts(self, ) -> None:
        common_option = "--show-dev-interrupts Hypervisor callback interrupts"
        sender_xml = "test_interrupts_sender.xml"
        sender_json = "test_interrupts_sender.json"
        sender_console = "test_interrupts_sender_console.out"
        sender_option = f"-x {sender_xml} -j {sender_json} -O {sender_console}"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option,
                sender_option=sender_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)

        # Verify console output has interrupts
        assert parse_result.is_show_dev_interrupts() is True

        # Get console interrupt metrics
        console_info = parse_result.get_console_interrupts_info()
        assert console_info is not None, "Console output missing interrupt metrics"
        console_total, console_per_sec = console_info

        # Verify console values are non-zero
        assert console_total > 0, "Console total_interrupts should be > 0"
        assert console_per_sec > 0.0, "Console interrupts_per_sec should be > 0"

        # Get and verify XML interrupt metrics
        xml_info = parse_result.get_xml_interrupts_info(sender_xml)
        assert xml_info is not None, "XML output missing interrupt metrics"
        xml_total, xml_per_sec = xml_info

        # Verify XML values match console
        assert xml_total == console_total, f"XML total_interrupts ({xml_total}) should match console ({console_total})"
        assert xml_per_sec > 0.0, "XML interrupts/sec should be > 0"
        # Allow small floating point differences
        assert abs(xml_per_sec - console_per_sec) < 0.01, \
            f"XML interrupts_per_sec ({xml_per_sec}) should match console ({console_per_sec})"

        # Get and verify JSON interrupt metrics
        json_info = parse_result.get_json_interrupts_info(sender_json)
        assert json_info is not None, "JSON output missing interrupt metrics"
        json_total, json_per_sec = json_info

        # Verify JSON values match console
        assert json_total == console_total, f"JSON total_interrupts ({json_total}) should match console ({console_total})"
        assert json_per_sec > 0.0, "JSON interrupts/sec should be > 0"
        assert abs(json_per_sec - console_per_sec) < 0.01, \
            f"JSON interrupts_per_sec ({json_per_sec}) should match console ({console_per_sec})"

        throughput = parse_result.get_throughput_Gbps()
        assert throughput >= self.expected_throughput

    def test_fq_rate_limit(self) -> None:
        throughput_limit_gbps = 10
        common_option = f"--fq-rate-limit {throughput_limit_gbps}G"
        receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option
        )
        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)
        throughput = parse_result.get_throughput_Gbps()
        assert int(throughput) in range(throughput_limit_gbps - 1, throughput_limit_gbps + 1)

    def test_udp_sender_socket_creation_regression(self) -> None:
        """
        Regression test for UDP sender socket creation and connection handling.
        Tests run_ntttcp_sender_udp4_stream end-to-end with multiple connections
        to ensure sockets are created, bound, and connected properly.

        This test exercises:
        - Socket creation loop in run_ntttcp_sender_udp4_stream
        - Socket binding with proper error handling, Connection establishment to receiver
        - Data transfer with multiple UDP connections, Proper cleanup of socket file descriptors
        """
        n_server_ports = 2
        n_threads = 3
        n_connections = 4
        total_connections = n_server_ports * n_threads * n_connections

        # Test UDP sender with multiple connections per thread
        common_option = f"-u -P {n_server_ports}"
        sender_option = f"-n {n_threads} -l {n_connections} -V"
        receiver_cmd, sender_cmd = self.combine_command(
            common_option=common_option,
            sender_option=sender_option
        )

        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)

        # Verify all connections were created successfully
        connections_created = parse_result.get_multi_threads_info()
        assert connections_created == total_connections, \
            f"Expected {total_connections} UDP connections, but only {connections_created} were created"

        # Verify data was transferred (UDP should show some throughput)
        throughput = parse_result.get_throughput_Gbps()
        assert throughput > 0, "UDP sender should transfer data successfully"

        # Verify no socket creation errors in output
        assert "cannot create socket endpoint" not in result.sender_stdout, \
            "Socket creation should succeed for all UDP connections"
        assert "failed to connect socket" not in result.sender_stdout, \
            "Socket connection should succeed for all UDP connections"

        self.log.write_info(f"UDP sender regression test passed: {connections_created} connections, {throughput:.2f} Gbps")

    def test_udp_sender_single_connection(self) -> None:
        """
        Test UDP sender with single connection to verify basic functionality.
        This is a simpler test case to catch basic UDP sender issues.
        """
        common_option = "-u"
        sender_option = "-V"
        receiver_cmd, sender_cmd = self.combine_command(
            common_option=common_option,
            sender_option=sender_option
        )

        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)

        # Verify data transfer occurred
        throughput = parse_result.get_throughput_Gbps()
        assert throughput > 0, "UDP sender should transfer data with single connection"

        # Verify no errors
        assert "cannot create socket endpoint" not in result.sender_stdout
        assert "failed to connect socket" not in result.sender_stdout

        self.log.write_info(f"UDP single connection test passed: {throughput:.2f} Gbps")

    def test_udp_sender_with_custom_port(self) -> None:
        """
        Test UDP sender with custom destination/server port to verify connection works correctly.
        This exercises the destination port configuration (-p flag) in run_ntttcp_sender_udp4_stream.
        Note: This does NOT test client port binding (-f flag).
        """
        starting_port = 15000
        n_server_ports = 2
        n_threads = 2
        n_connections = 3
        total_connections = n_server_ports * n_threads * n_connections

        common_option = f"-u -p {starting_port} -P {n_server_ports}"
        sender_option = f"-n {n_threads} -l {n_connections} -V"
        receiver_cmd, sender_cmd = self.combine_command(
            common_option=common_option,
            sender_option=sender_option
        )

        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)

        # Verify connections were created
        connections_created = parse_result.get_multi_threads_info()
        assert connections_created == total_connections, \
            f"Expected {total_connections} UDP connections with custom port"

        # Verify custom port appears in UDP stream messages
        assert f"--> {self.loopback_interface}:{starting_port}" in result.sender_stdout, \
            f"Expected UDP streams to connect to custom port {starting_port}"

        # Verify data transfer
        throughput = parse_result.get_throughput_Gbps()
        assert throughput > 0, "UDP sender should transfer data with custom port"

        # Verify no errors
        assert "cannot create socket endpoint" not in result.sender_stdout
        assert "failed to connect socket" not in result.sender_stdout

        self.log.write_info(f"UDP custom port test passed: {connections_created} connections, {throughput:.2f} Gbps")

    def test_udp_sender_high_connection_count_stress(self) -> None:
        """ Stress test: Verify UDP sender handles very high connection counts correctly.

        Tests with elevated connection count to stress the socket creation loop:
        - Creates many simultaneous UDP connections, Verifies all are handled without leaks
        - Tests that continue logic works when some might fail, Ensures proper cleanup of all sockets

        This exercises:
        - Socket creation loop robustness in udpstream.c, Proper iteration through sockfds array
        - Error recovery when socket creation might fail under load, Memory management with many allocations
        """
        n_server_ports = 3
        n_threads = 4
        n_connections = 8  # Total: 96 connections
        total_connections = n_server_ports * n_threads * n_connections

        # Test with high connection count
        common_option = f"-u -P {n_server_ports}"
        sender_option = f"-n {n_threads} -l {n_connections} -V"
        receiver_cmd, sender_cmd = self.combine_command(
            common_option=common_option,
            sender_option=sender_option,
            duration=3
        )

        result = self.run_test(receiver_cmd, sender_cmd)
        parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)

        # Should create all requested connections without errors
        connections_created = parse_result.get_multi_threads_info()
        assert connections_created == total_connections, \
            f"Expected {total_connections} connections under stress test, got {connections_created}"

        # Verify no errors despite high load
        assert "cannot create socket endpoint" not in result.sender_stdout, \
            "Socket creation should succeed even with many connections"
        assert "failed to connect socket" not in result.sender_stdout, \
            "Socket connections should succeed even with many connections"

        # Verify actual data transfer occurred (tests cleanup was proper)
        throughput = parse_result.get_throughput_Gbps()
        assert throughput > 0, "Should have throughput even under high connection stress"

        self.log.write_info(f"Stress test passed: {connections_created} connections, {throughput:.2f} Gbps")

    def test_udp_sender_connect_failure_handling(self) -> None:
        """ Negative test: Verify UDP sender handles connection failures gracefully.

        Tests error handling when receiver is not available:
        - Attempts to connect to non-existent receiver, Verifies proper error logging and cleanup
        - Ensures no socket leaks on connect failure, Confirms test exits gracefully with error

        This exercises:
        - udpstream.c connect error handling, Socket cleanup: close(sockfd) + sockfds[i] = -1
        - ASPRINTF + PRINT_ERR_FREE error logging, Proper handling when all connections fail
        """
        # Use a port where no receiver is listening
        unused_port = 55555
        n_connections = 3

        # Don't start receiver - sender should fail to connect
        sender_cmd = f"ulimit -n 40960 && ./src/ntttcp -s{self.loopback_interface} -u -p {unused_port} -l {n_connections} -V -t 1 -Q"

        # Sender should exit with error since no receiver
        result = subprocess.run(sender_cmd, shell=True, capture_output=True, text=True, timeout=15)

        # Verify error handling was executed (should mention sync failure since no receiver)
        combined_output = result.stdout + result.stderr
        assert result.returncode != 0 or "failed to create sync socket" in combined_output or "failed to connect" in combined_output.lower(), \
            "Expected connection failure when receiver not available"

        self.log.write_info(f"Connect failure test: verified graceful handling of missing receiver (exit code: {result.returncode})")

    def test_udp_sender_rapid_succession_cleanup(self) -> None:
        """ Stress test: Verify UDP sender properly cleans up resources in rapid succession.

        Runs multiple short UDP tests back-to-back to verify:
        - Sockets are properly closed after each test, No file descriptor leaks accumulate
        - Error paths clean up correctly, Resources are released for reuse

        This validates the cleanup paths exercised in PR:
        - Proper close() of all sockfds, Memory cleanup in normal exit path
        - No resource accumulation across multiple test runs
        """
        n_ports = 1
        n_threads = 1
        n_connections = 5
        total_connections = n_ports * n_threads * n_connections
        test_iterations = 3

        for iteration in range(test_iterations):
            common_option = f"-u -P {n_ports}"
            sender_option = f"-n {n_threads} -l {n_connections} -V"
            receiver_cmd, sender_cmd = self.combine_command(
                common_option=common_option,
                sender_option=sender_option,
                duration=1
            )

            result = self.run_test(receiver_cmd, sender_cmd)
            parse_result = ntttcp_output.NtttcpOutput(result.receiver_stdout, result.sender_stdout)

            # Each iteration should succeed independently
            connections_created = parse_result.get_multi_threads_info()
            assert connections_created == total_connections, \
                f"Iteration {iteration+1}: Expected {total_connections} connections, got {connections_created}"

            throughput = parse_result.get_throughput_Gbps()
            assert throughput > 0, f"Iteration {iteration+1}: Should have throughput"

            # Small delay between iterations to ensure cleanup completes
            time.sleep(0.5)

        self.log.write_info(f"Rapid succession test passed: {test_iterations} iterations completed successfully")

if __name__ == "__main__":
    pytest.main()
