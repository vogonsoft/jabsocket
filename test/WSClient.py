#!/usr/bin/python

import socket
import select
import random
import time
import base64

random.seed(time.time())

class WSException(Exception):
	def __init__(self, e):
		self.message = str(e)
	def __str__(self):
		return self.message

class ConnectException(WSException):
	def __init__(self, e):
		super(ConnectException, self).__init__(e)

class SendException(WSException):
	def __init__(self, e):
		super(SendException, self).__init__(e)

class Timeout(WSException):
	def __init__(self, e):
		super(Timeout, self).__init__(e)

class HandshakeKeyError(WSException):
	def __init__(self, e):
		super(Timeout, self).__init__(e)

def randString(*par):
	if len(par) == 0:
		length = 8
	else:
		length = par[0]
	s = ''
	for i in range(length):
		c = chr(random.randint(0, 255))
		s = s + c
	return s

def encode_number(n):
	e = ''
	while n > 0:
		r = n % 256
		e = chr(r) + e
		n = n / 256
	if len(e) == 0:
		e = chr(0)
	return e

def decode_number(buffer):
	n = 0
	i = len(buffer) - 1
	while i >= 0:
		d = ord(buffer[i])
		n = 256 * n + d
	return n

def encode_length(length):
	if length < 126:
		return ''
	if length < 2 * 16:
		e = encode_number(length)
		return e
	e = encode_number(length)
	e = (chr(0) * 8) + e
	e = e[-8:]
	return e

def parse_header(lines):
	# parses header lines
	# e.g. if lines=["Sec-WebSocket-Protocol: xmpp, smtp"]
	# the function returns dictionary
	# {"Sec-WebSocket-Protocol": ["xmpp", "smtp"]}
	d = {}
	for line in lines:
		l = line.split(':', 1)
		if len(l) == 2:
			key = l[0]
			value = l[1]
			values = value.split()
			d[key] = values
	return d

class Client:
	def __init__(self):
		self.socket = None
		self.buffer = '' # buffer used by recvLine and recvFrame
	
	def open(self, host, port):
		try:
			self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		except Exception, e:
			raise WSException(e)
		try:
			self.socket.connect((host, port))
		except Exception, e:
			self.socket = None
			raise ConnectException(e)
		
		self.p = select.poll()
		self.p.register(self.socket.fileno(), select.POLLIN | select.POLLERR | \
			select.POLLHUP)
	
	def send(self, data):
		# print 'sending data'
		if not self.socket:
			raise WSException('Error: socket not open')
		try:
			self.socket.sendall(data)
		except Exception, e:
			raise SendException(e)

	# recv is public function that returns any data left over in
	# buffer.
	def recv(self, n, timeout):
		if not self.socket:
			raise WSException('Error: socket not open')
		if len(self.buffer) > 0:
			s = self.buffer
			self.buffer = ''
			return s
		return self._recv(n, timeout)

	# _recv is private function that doesn't check buffer.
	# It is called by recvLine and recvFrame.
	def _recv(self, n, timeout):
		if not self.socket:
			raise WSException('Error: socket not open')
		results = self.p.poll(timeout)
		if len(results) == 0:
			raise Timeout('Waiting for data')
		if results[0][1] != select.POLLIN:
			raise SocketError()
		return self.socket.recv(n)
	
	def recvLine(self, timeout):
		# recvLine tries to read a line (terminated by CR+LF). If it is
		# successful, it returns the line, leaving any remaining bytes read
		# in self.buffer
		if not self.socket:
			raise WSException('Error: socket not open')
		crlf = '\r\n'
		while True:
			if crlf in self.buffer:
				s = self.buffer.split(crlf, 1)
				self.buffer = s[1]
				return s[0]
			r = self._recv(1024, timeout)
			if len(r) == 0:
				return ''
			self.buffer = self.buffer + r
				
	
	def close(self):
		if not self.socket:
			raise WSException('Error: socket not open')
		self.socket.close()
		self.socket = None

	def sendHandshakeRequest(self, **params):
		'''Sends handshake request to the server.
		   Optional parameters:
		     - resource
		     - protocol (e.g. HTTP/1.1)
		     - subprotocol (e.g. xmpp)
		     - key (for Sec-WebSocket-Key)
		     - version (for Sec-WebSocket-Version)
		     - origin (e.g. http://example.com)
		'''
		if not self.socket:
			raise WSException('Error: socket not open')
		try:
			d = params
			resource = d.get('resource', '/')
			protocol = d.get('protocol', 'HTTP/1.1')
			host = d['host']
			key = d.get('key', base64.encodestring(randString(8)))
			key = key[:-1] # Remove trailing LF
			subprotocol = d['subprotocol']
			version = d.get('version', '13')
			origin = d.get('origin', 'http://' + host)
		except KeyError, e:
			raise HandshakeKeyError(e)
		self.send(('GET %s %s\r\n' +
			'Host: %s\r\n' +
			'Upgrade: websocket\r\n' +
			'Connection: Upgrade\r\n' +
			'Sec-WebSocket-Key: %s\r\n' +
			'Sec-WebSocket-Protocol: %s\r\n' + 
			'Sec-WebSocket-Version: %s\r\n' +
			'Origin: %s\r\n\r\n') % (resource, protocol, host, key, subprotocol,
				version, origin))
	def recvHandshakeResponse(self, timeout):
		lines = []
		while True:
			r = self.recvLine(timeout)
			# print 'r=%s' % r
			if r == None:
				return None
			# print 'Received line: %s' % r
			# print 'lines=%s' % lines
			lines.append(r)
			if len(r) == 0:
				return lines
	
	def sendFrame(self, data, **params):
		d = params
		fin = d.get('fin', 1)
		masked = d.get('masked', 1)
		if masked == 1:
			try:
				mask = d['mask']
			except KeyError:
				raise WSException('mask needed')
		opcode = d.get('opcode', 1)
		frame = ''
		frame = frame + chr(fin * 0x80 + opcode)
		length = len(data)
		if length < 126:
			frame = frame + chr(masked * 0x80 + length)
		else:
			length_enc = encode_length(length)
			if len(length_enc) == 2:
				frame = frame + chr(masked * 0x80 + 126)
			else:
				frame = frame + chr(masked * 0x80 + 127)
			frame = frame + length_enc
		if masked == 1:
			frame = frame + mask
			i = 0
			ind = 0
			while i < len(data):
				frame = frame + chr(ord(mask[ind]) ^ ord(data[i]))
				ind = (ind + 1) % 4
				i = i + 1
		else:
			frame = frame + data
		self.send(frame)

	def recvFrame(self, timeout):
		# recvLine tries to read a frame. If it is successful, it returns the
		# frame as dictionary {'fin': fin, 'opcode': opcode, 'data': data},
		# leaving any remaining bytes read in self.buffer.
		if not self.socket:
			raise WSException('Error: socket not open')
		while True:
			if len(self.buffer) > 1:
				by0 = ord(self.buffer[0])
				by1 = ord(self.buffer[1])
				if (by0 & 0x70) != 0:
					raise WSException('RSV1-3 are not 0')
				fin = (by0 & 0x80) >> 7
				opcode = by0 & 0x0f
				masked = (by1 & 0x80) >> 7
				l = by1 & 0x7f
				# print 'recvFrame: l=%d' % l
				if l < 126:
					header_length = 2
				elif l == 126:
					header_length = 4
				else:
					header_length = 10
				# print 'recvFrame: header_length=%d' % header_length
				if len(self.buffer) >= header_length:
					if l < 126:
						length = l
					elif l == 126:
						length = decode_number(self.buffer[2:4])
					else:
						length = decode_number(self.buffer[2:10])
					min_length = header_length + length
					payload_start = header_length
					if masked:
						min_length = min_length + 4
						payload_start = payload_start + 4
					# print 'recvFrame: min_length=%d' % min_length
					if len(self.buffer) >= min_length:
						data = self.buffer[payload_start:payload_start + length]
						if masked:
							mask = self.buffer[min_length:min_length + 4]
							unmask(data, mask)
						self.buffer = self.buffer[payload_start + length:]
						return {'fin': fin, 'opcode': opcode, 'data': data}
			r = self._recv(1024, timeout)
			# print 'recvFrame: got %d bytes: %s' % (len(r), r)
			if len(r) == 0:
				return None
			self.buffer = self.buffer + r

