import WSClient
import pytest

class TestProtocol:
	def test_ping_pong(self, client):
		client.sendHandshakeRequest(host='localhost', subprotocol='xmpp')
		response = client.recvHandshakeResponse(1000)
		
		# Send Ping frame.
		client.sendFrame(
			'abcd', fin=1, masked=1, opcode=9, mask='aaaa')

		# Receive Pong frame as the response.
		r = client.recvFrame(1000)
		assert r['data'] == 'abcd'
		assert r['fin'] == 1
		assert r['opcode'] == 9

	def test_close(self, client):
		client.sendHandshakeRequest(host='localhost', subprotocol='xmpp')
		response = client.recvHandshakeResponse(1000)

		# Send text frame.
		client.sendFrame(
			'abcd', fin=0, masked=1, opcode=1, mask='aaaa')

		# We don't expect a response (because fin=0).
		with pytest.raises(WSClient.Timeout):
			r = client.recvFrame(200)
		
		# Send Close frame.
		client.sendFrame(
			'ABCD', fin=1, masked=1, opcode=8, mask='xyzt')

		# Receive Close frame as the response.
		r = client.recvFrame(1000)
		assert r['data'] == 'ABCD'
		assert r['fin'] == 1
		assert r['opcode'] == 8

