#!/usr/bin/python
'''
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

'''

from optparse import OptionParser
import sys
import os
import time
import random

def random_mask():
	'''
	random_mask generates a random 4-byte array that will be used
	as a mask for WebSocket frame
	'''
	mask = []
	for i in range(4):
		mask.append( chr( random.randint(0, 0xff) ) )
	return mask

def serialize(num, len):
	'Serialize num to an array of len bytes (big-endian)'
	t = []
	while num > 0:
		r = num % 0x100
		t = [r] + t
		num = num / 0x100
	t = ([0x00] * len) + t
	return t[-len:]

def generate_frame(data, **options):
	'''
	generate_frame generates a WebSocket frame as
	an array of bytes
	'''
	# print 'generate_frame: data=%s' % data
	masked = False
	fragment = False
	frame_type = None
	mask = None
	if 'mask' in options:
		# print 'mask: %s' % options['mask']
		mask = options['mask']
	if 'masked' in options:
		# print 'masked: %s' % options['masked']
		masked = True
	if 'fragment' in options:
		fragment = options['fragment']
		# print 'fragment:', fragment
	if masked and ('mask' not in options):
		raise Exception('masked=True and mask not specified')
	if 'type' in options:
		frame_type = options['type']
	# print 'frame type: %s' % frame_type
	
	if not frame_type:
		opcode = 0
	else:
		opcodes = {'text': 1, 'binary': 2, 'close': 8, 'ping': 9, 'pong': 10}
		if frame_type not in opcodes:
			raise Exception('unknown frame type %s' % frame_type)
		opcode = opcodes[frame_type]
	# print 'opcode: %d' % opcode
	
	# byte 0: FIN, opcode
	if fragment:
		b0 = 0
	else:
		b0 = 0x80
	b0 = b0 + opcode
	# print 'byte 0: %x' % b0
	
	b = []
	b.append(b0)
	
	# byte 1: MASK, payload length (7 bits)
	b1 = 0
	if masked:
		b1 = 0x80
	if len(data) < 126:
		l0 = len(data)
		l = []
	elif len(data) < (2 << 32 - 1):
		l0 = 126
		l = serialize(len(data), 2)
	else:
		l0 = 127
		l = serialize(len(data), 8)
	b1 = b1 + l0
	b.append(b1)
	# print 'byte 1: %x' % b1
	
	b = b + l
	
	if masked:
		b = b + mask
		index = 0
		for d in data:
			d1 = chr(ord(d) ^ ord(mask[index]))
			b.append(d1)
			index = index + 1
			if index == 4:
				index = 0
	else:
		b = b + data
	
	return b

def create_c_array(name, data):
	'''
	create_c_array returns a string that is suitable for pasting into a C
	source file.
	E.g.:
	byte data0 = {
	    0x1, 0x85, 0xa5, 0x55, 0xc0, 0x55, 0xc4, 0x37, 0xa3, 0x31, 0xc0
	};
	'''
	s = 'byte %s[] = {\n\t' % name
	j = 0
	for i in range(len(data)):
		c = data[i]
		if type(c) == str:
			c = ord(c)
		s1 = hex(c)
		s = s + s1
		if i + 1 < len(data):
			s = s + ','
		j = j + 1
		if j < 16:
			s = s + ' '
		else:
			s = s + '\n\t'
			j = 0
	if j > 0:
		s = s + '\n'
	s = s + '};\n'
	return s

def main():
	parser = OptionParser()
	parser.add_option('-b', '--binary', action='store_true', dest='binary',
		default=False, help='generate binary frames')
	parser.add_option('-m', '--maxsize', dest='maxsize', type='int',
		default=False, help='maximum size of payload per frame')
	parser.add_option('--prefix', dest='prefix',
		default='data', help='prefix of variable that stores a frame')
	parser.add_option('--unmasked', default=False, action='store_true',
		help='generate unmasked frames')
	parser.add_option('--type', type='choice', choices=['text', 'binary',
		'ping', 'pong', 'close'], default='text',
		help='frame type (text|binary|ping|pong|close)')

	(options, args) = parser.parse_args()
	read_stdin = False
	if len(args) > 0:
		filename = args[0]
		print 'filename: %s' % filename
		fin = open(filename, 'r')
	else:
		print 'reading stdin'
		fin = sys.stdin
		read_stdin = True
	if options.binary:
		print 'Generating binary frames'
	else:
		print 'Generating text frames'
	if options.maxsize:
		print 'Maximum size of payload per frame: %d' % options.maxsize
	else:
		print 'Sending message in a single frame'
	print 'Prefix: %s' % options.prefix
	if options.unmasked:
		print 'Generating unmasked frames'
	else:
		print 'Generating masked frames'
	print 'Frame type: %s' % options.type
	
	random.seed(time.time())
	
	# Read from fin (file input)
	data = fin.read()
	if options.maxsize:
		first = True
		i = 0
		while len(data) > 0:
			if len(data) < options.maxsize:
				chunk = data
				data = []
			else:
				chunk = data[:options.maxsize]
				data = data[options.maxsize:]
			if len(data) > 0:
				fragment = True
			else:
				fragment = False
			# print '   read %d bytes, fragment=%s' % (len(chunk), fragment)
			# print '     (data=%s)' % data
			if first:
				b = generate_frame(chunk, masked=not options.unmasked,
					fragment=fragment,
					mask=random_mask(), type=options.type)
			else:
				b = generate_frame(chunk, masked=not options.unmasked,
					fragment=fragment,
					mask=random_mask())
			s = create_c_array('data' + str(i), b)
			print s
			first = False
			i = i + 1
	else:
		b = generate_frame(data, masked=not options.unmasked,
			mask=['x', 'y', 'z', 't'], type=options.type)
		s = create_c_array('data0', b)
		print s

if __name__ == '__main__':
	main()

