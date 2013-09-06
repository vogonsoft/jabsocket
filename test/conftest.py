#!/usr/bin/python

import pytest
import WSClient
import util
import time

def pytest_addoption(parser):
	parser.addoption('--host', action='store', default='localhost',
		help='Host to connect to')
	parser.addoption('--port', action='store', default='5000',
		help='Port to connect to')
	parser.addoption('--dontrunserver', action='store_true',
		help='Don\'t run the server')
	parser.addoption('--server', action='store', default='jabsocket',
		help='Path to server')
	parser.addoption('--jsconfig', action='store', default='jabsocket.conf',
		help='Path to configuration file')

@pytest.fixture()
def client(request):
	my_client = WSClient.Client()
	host = pytest.config.getvalue('host')
	port = int(pytest.config.getvalue('port'))
	dontrunserver = pytest.config.getvalue('dontrunserver')
	server = pytest.config.getvalue('server')
	jsconfig = pytest.config.getvalue('jsconfig')
	if not dontrunserver:
		pid = util.start(server, server, '-c', jsconfig)
	time.sleep(0.05) # Give the child process a chance to run
	my_client.open(host, port)
	def fin():
		my_client.close()
		if not dontrunserver:
			util.stop(pid)
	request.addfinalizer(fin)
	return my_client

