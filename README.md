# NTTTCP-for-Linux

## Summary

A multiple-thread based Linux network throughput benchmark tool.

## Features

* Multiple threads to send/receive data. By default, Receiver ("-r") uses 16 threads and Sender ("-s") uses 64 threads to exchange data.

* Support cpu affinity.

* Support running in background (daemon).

* Support Sender and Receiver sync mode by default. Use "-N" (no_sync) to disable the sync.

* Only support TCP mode; and support both ipv4 and ipv6.


## Getting Started


### Building NTTTCP-for-Linux ###

	make; make install

### Usage
	
	ntttcp -h

### Known issues

* UDP is not supported. 

### Example run

To measure the network performance between two multi-core serves running SLES 12, NODE1 (192.168.4.1) and NODE2 (192.168.4.2), connected via a 40 GigE connection. 

On NODE1 (the receiver), run:

./ntttcp -r

(Translation: Run ntttcp as a receiver with default setting. The default setting includes: with 16 threads created and run across all CPUs, allocating 64K receiver buffer, and run for 60 seconds.)

And on NODE2 (the sender), run:

./ntttcp.exe -s192.168.4.1

(Translation: Run ntttcp as a sender, with default setting. The default setting includes: with 64 threads created and run across all CPUs, allocating 128KB sender buffer, and run for 60 seconds.)

Using the above parameters, the program returns results on both the sender and receiver nodes, correlating network communication to CPU utilization.  

Example receiver-side output from a given run (which showcases 37.66 Gbps throughput):

```
NODE1:/home/simonxiao/ntttcp-for-linux/src # ./ntttcp -r
NTTTCP for Linux 1.1.0
---------------------------------------------------------
13:44:25 INFO: Network activity progressing...
13:45:25 INFO:  Thread  Time(s) Throughput
13:45:25 INFO:  ======  ======= ==========
13:45:25 INFO:  0        60.00   1.83Gbps
13:45:25 INFO:  1        60.00   2.51Gbps
13:45:25 INFO:  2        60.00   2.54Gbps
13:45:25 INFO:  3        60.00   1.94Gbps
13:45:25 INFO:  4        60.00   1.93Gbps
13:45:25 INFO:  5        60.00   2.09Gbps
13:45:25 INFO:  6        60.00   2.15Gbps
13:45:25 INFO:  7        60.00   3.23Gbps
13:45:25 INFO:  8        60.00   1.77Gbps
13:45:25 INFO:  9        60.00   3.30Gbps
13:45:25 INFO:  10       60.00   2.97Gbps
13:45:25 INFO:  11       60.00   3.65Gbps
13:45:25 INFO:  12       60.00   1.67Gbps
13:45:25 INFO:  13       60.00   1.86Gbps
13:45:25 INFO:  14       60.00   1.61Gbps
13:45:25 INFO:  15       60.00   2.61Gbps
---------------------------------------------------------
13:45:25 INFO: test duration    :60.00 seconds
13:45:25 INFO: total bytes      :282418471264
13:45:25 INFO:   throughput     :37.66Gbps
13:45:25 INFO: total cpu time   :166.11%
13:45:25 INFO:   user time      :39.89%
13:45:25 INFO:   system time    :126.23%
13:45:25 INFO:   cpu cycles     :192000045567
13:45:25 INFO: cycles/byte      :0.68
---------------------------------------------------------
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
