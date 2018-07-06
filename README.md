# NTTTCP-for-Linux

## Summary

A multiple-thread based Linux network throughput benchmark tool.

## Features

* Multiple threads to send/receive data. By default, Receiver ("-r") uses 16 threads and Sender ("-s") uses 64 threads to exchange data.

* Support cpu affinity.

* Support running in background (daemon).

* Support Sender and Receiver sync mode by default. Use "-N" (no_sync) to disable the sync.

* Support testing with multiple clients mode (use '-M' on Receiver, and '-L' on the last Sender).

* Support select() by default, and epoll() (use '-e' on Receiver).

* Support both TCP (by default), and UDP ('-u') tests.

* Support pin TCP server or client port (use '-p' on Receiver or '-f' on Sender).

* Support test Warmup ('-W') and Cooldown ('-C').

* Support reporting TCP retransmit ('-R').

* Support writing log into XML file ('-x').


## Getting Started


### Building NTTTCP-for-Linux ###

	make; make install

### Usage
	
	ntttcp -h

### Known issues

 

### Example run

To measure the network performance between two multi-core serves running SLES 12, NODE1 (192.168.4.1) and NODE2 (192.168.4.2), connected via a 40 GigE connection. 

On NODE1 (the receiver), run:
```
./ntttcp -r
```
(Translation: Run ntttcp as a receiver with default setting. The default setting includes: with 16 threads created and run across all CPUs, allocating 64K receiver buffer, and run for 60 seconds.)

And on NODE2 (the sender), run:
```
./ntttcp -s192.168.4.1
```
(Translation: Run ntttcp as a sender, with default setting. The default setting includes: with 64 threads created and run across all CPUs, allocating 128KB sender buffer, and run for 60 seconds.)

Using the above parameters, the program returns results on both the sender and receiver nodes, correlating network communication to CPU utilization.  

Example receiver-side output from a given run (which showcases 37.66 Gbps throughput):

```
NODE1:/home/simonxiaoss/ntttcp-for-linux/src # ./ntttcp -s 10.0.0.1 -W 1 -t 10 -C 1
NTTTCP for Linux 1.3.4
---------------------------------------------------------
22:36:30 INFO: Network activity progressing...
22:36:30 INFO: 64 threads created
22:36:31 INFO: Test warmup completed.
22:36:42 INFO: Test run completed.
22:36:42 INFO: 64 connections tested
22:36:42 INFO: #####  Totals:  #####
22:36:42 INFO: test duration    :10.36 seconds
22:36:42 INFO: total bytes      :30629953536
22:36:42 INFO:   throughput     :23.65Gbps
22:36:42 INFO: cpu cores        :20
22:36:42 INFO:   cpu speed      :2394.455MHz
22:36:42 INFO:   user           :3.60%
22:36:42 INFO:   system         :3.44%
22:36:42 INFO:   idle           :91.57%
22:36:42 INFO:   iowait         :0.00%
22:36:42 INFO:   softirq        :1.38%
22:36:42 INFO:   cycles/byte    :1.37
22:36:42 INFO: cpu busy (all)   :159.81%
---------------------------------------------------------
22:40:52 INFO: Test cooldown is in progress...
22:40:52 INFO: Test cycle finished.
```

# Related topics

1. [NTTTCP](https://gallery.technet.microsoft.com/NTttcp-Version-528-Now-f8b12769)

2. [Microsoft Server & Cloud Blog](http://blogs.technet.com/b/server-cloud/)

3. [HyperV Linux Integrated Services Test](https://github.com/LIS/lis-test)


## Terms of Use

By downloading and running this project, you agree to the license terms of the third party application software, Microsoft products, and components to be installed. 

The third party software and products are provided to you by third parties. You are responsible for reading and accepting the relevant license terms for all software that will be installed. Microsoft grants you no rights to third party software.


## License

```
The MIT License (MIT)

Copyright (c) 2015 Microsoft Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
