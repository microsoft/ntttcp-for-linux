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
