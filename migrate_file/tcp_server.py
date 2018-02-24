#!/usr/bin/env python
import SocketServer

class MyTCPHandler(SocketServer.BaseRequestHandler):
	def handle(self):
		while True:
			self.data = self.request.recv(1024).strip()
			if self.data:
				print "{} wrote:".format(self.client_address[0])
				print self.data
			else:
				break;

if __name__ == "__main__":
	HOST, PORT = "localhost", 9999
	server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
	server.serve_forever()
