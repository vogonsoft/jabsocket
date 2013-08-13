Utilities
=========

This directory contains various utilities.

makewsmessage.py
----------------

This script generates a WebSocket frame in the form of a C array declaration
that can be pasted directly into a C source file.

Example output:

	byte data0[] = {
		0x81, 0x84, 0x4b, 0xfc, 0xec, 0x7b, 0x2a, 0x9e, 0x8f, 0x71 
	};

	Usage: makewsmessage.py [options]

	Options:
	  -h, --help            show this help message and exit
	  -b, --binary          generate binary frames
	  -m MAXSIZE, --maxsize=MAXSIZE
		                    maximum size of payload per frame
	  --prefix=PREFIX       prefix of variable that stores a frame
	  --unmasked            generate unmasked frames
	  --type=TYPE           frame type (text|binary|ping|pong|close)

If a file is specified as a positional argument, then its contents are
used as data. Otherwise, standard input is used.

Example using standard input:

	$ echo "abc" | ./makewsmessage.py --type=text -m 5
	reading stdin
	Generating text frames
	Maximum size of payload per frame: 5
	Prefix: data
	Generating masked frames
	Frame type: text
	byte data0[] = {
		0x81, 0x84, 0x4b, 0xfc, 0xec, 0x7b, 0x2a, 0x9e, 0x8f, 0x71 
	};

