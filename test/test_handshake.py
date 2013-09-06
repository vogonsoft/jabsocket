import WSClient

class TestHandshake:
	def test_correct_handshake(self, client):
		client.sendHandshakeRequest(host='localhost', subprotocol='xmpp')
		response = client.recvHandshakeResponse(1000)
		d = WSClient.parse_header(response[1:])
		assert 'xmpp' in d['Sec-WebSocket-Protocol']

	def test_bad_version(self, client):
		client.sendHandshakeRequest(host='localhost', subprotocol='xmpp',
			version='12')
		response = client.recvHandshakeResponse(1000)

		# response[0] is '426 Upgrade Required'
		assert '426' in response[0]

