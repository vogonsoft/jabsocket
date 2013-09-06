jabsocket
=========

XMPP-over-WebSocket connection manager

Introduction
------------

jabsocket is an XMPP over WebSocket connection manager written in C. This means
that it is a server application that enables connection between a web
browser and an XMPP server.

The connection with JavaScript code running in a browser is over WebSocket.
jabsocket aims to support Websocket as defined in RFC-6455 and XMPP sub-protocol
for WebSocket as defined in the draft document
[An XMPP Sub-protocol for WebSocket](https://tools.ietf.org/html/draft-moffitt-xmpp-over-websocket-03)

NOTE: At this point, jabsocket is in alpha stage. Please see TODO for areas that
need work.

Contact
-------

For any problem, idea or suggestion, please contact the author, Aleksandar
Janicijevic, at aleks@vogonsoft.com.

License
-------

This software is released under the MIT License. See file LICENSE for
details.

Source code
-----------

You can get the latest source by:
	git clone git@github.com:vogonsoft/jabsocket.git

Quick Start Guide
-----------------

1. To run as daemon, type:
	jabsocket -c jabsocket.conf

2. For quick help, type:
	jabsocket --help

Prerequisites
-------------

jabsocket was written to run on POSIX systems. At the moment it has only been
tested on Linux.

External dependencies:
* libevent 2.0.21
* Expat 2.1.0
* libyaml 0.1.3-1
* libssl 0.9.8

To build from source, you need to have CMake (version 2.8.0 or higher)
installed. If your operating system does not have it and your distribution
does not have an installation package, please see at http://www.cmake.org
how to install CMake.

Building
--------

You first have to use CMake to generate the Makefile. To use the default
configuration (which is release):

	cmake .

To generate Makefile for debug:

	cmake -D CMAKE_BUILD_TYPE=DEBUG .

To generate Makefile for release

	cmake -D CMAKE_BUILD_TYPE=RELEASE .

After you generated the Makefile, run:

	make

To run make verbosely, run:

	make VERBOSE=1

Building binary package:

	make package

Building source package:

	make package_source

Installation
------------

Run this as a superuser or use sudo:

	make install

Unit test
---------

	make check

Integration test
----------------

Integration test is written to work with Python testing library py.test.
After you have built jabsocket binary, if you have py.test installed,
run the test like this:

	py.test test

The tests are defined in the subdirectory test and currently they only test
two things: WebSocket handshake and sending and receiving WebSocket frames.

The test suite has some flexibility: by default it runs jabsocket that was
built in the project root directory and uses jabsocket.conf for configuration.
You can choose to run jabsocket by yourself and then tell the test suite not
to run it; or, if you let the test suite run the binary, you can customize the
path to the binary, the configuration file, host (default is localhost) and
port (default is 5000, because that is the port defined in jabsocket.conf).
You can get the options by running

	py.test test --help

It will display, among the predefined py.test options:

	custom options:
		--host=HOST         Host to connect to
		--port=PORT         Port to connect to
		--dontrunserver     Don't run the server
		--server=SERVER     Path to server
		--jsconfig=JSCONFIG
				            Path to configuration file

Documentation
-------------

The documentation is in doc subdirectory.

Author
------

Aleksandar Janicijevic, aleks@vogonsoft.com

Acknowledgements
----------------

* Base64 encode/decode was adapted from
  ryyst (http://stackoverflow.com/users/282635/ryyst)
* libyaml is copyright (c) 2006 Kirill Simonov
* I learned the basics of libyaml from libyaml tutorial at
  http://wpsoftware.net/andrew/pages/libyaml.html
  written by Andrew Poelstra.
* libexpat (home page http://www.libexpat.org, project page
  http://sourceforge.net/projects/expat) is James Clark's XML parser library.
* libevent (http://libevent.org) was written by Nick Mathewson and
  Niels Provos.
* CuTest (http://cutest.sourceforge.net) unit-testing library is
  Copyright © 2002-2003, Asim Jalis

