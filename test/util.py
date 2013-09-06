#!/usr/bin/python

import os

def start(cmd, *args):
	pid = os.fork()
	if pid == 0:
		# Child process
		os.execv(cmd, args)
	return pid

def stop(pid):
	os.kill(pid, 9)
	(pid, status) = os.waitpid(pid, 0)
	return status

