#!/usr/bin/env python

import socket
import logging
import time
import platform
import os
import signal

class Server:
	
	
	def __init__(self, port = 8000):
		self.host = 'localhost'
		self.port = port

	def start_server(self):
		try:
			self.socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
			self.socket.bind((self.host,self.port))
		except Exception as e:
			logging.error("ERROR WITH CREATING SOCKET:\n" + str(e))
			self.shutdown_server()
			exit(1)

		logging.info("Starting server on port " + str(self.port))
		self.listen()

	def gen_headers(self, code):
	
		header = '127.0.0.1'
		
		if code	== 1:
			header = 'HTTP/1.1 200 OK\n'
			header += 'Connection: close\n'
			#header += 'Keep-Alive: timeout=5, max=100\n'
			header += 'Refresh: {0};\n'.format(self.time)
		else:
			if code == 200:
				header = 'HTTP/1.1 200 OK\n'
			elif code == 404:
				header = 'HTTP/1.1 404 Not Found\n'
			elif code == 405:
				header = 'HTTP/1.1 405 Method Not Allowed\n'
			header += 'Connection: close\n'

		date = time.strftime("%a, %d %b %Y %H:%M:%S", time.localtime())
		header += 'Date: ' + date +'\n'
		header += 'Content-Type: text/html; charset=utf-8\n'
		header += 'Server: Simple-Python-HTTP-Server(xgiesl00)\n\n'

			
		logging.debug("GENERATED HEADER: " + header)
		return header
	
	
	#shutdown the socket
	def shutdown_server(self):
	    try:
	    	logging.info("Shutting down the server")
	    	s.socket.shutdown(socket.SHUT_RDWR)
	    except Exception as e:
	    	logging.error("Error with closing server:\n" + str(e))
	
	#listen for connections
	def listen(self):
		
	
		#get machine info
		info = os.uname()
		hostname = info[1]
		cpu_info = info[4]
	
		logging.debug(hostname)
		logging.debug(cpu_info)
		#logging.debug(cpu_load)
	
		#exit(1)
		#run socket
		while True:

			logging.debug("Waiting for connection")
			#set max qued connections to 5
			self.socket.listen(1)

			conn, addr = self.socket.accept()
			
			logging.debug("[+] Connecting by {0}:{1} ({2})".format(addr[0], addr[1], time))
			
			data = conn.recv(1024).decode('utf-8')
			
			logging.debug("Received:\n {0}".format(str(data)))
			#check if is a GET method
			method = data.split(' ')[0]

			logging.debug("Method: " + method)
			response_content = ''
			response_header = gen_headers(404)


			path = data.split(' ')
			if len(path) > 1:
				path = path[1]

			#logging.debug('path = ' + path)
	
			if method == 'GET':
				if path == '/hostname':
					response_header = self.gen_headers(200)
					response_content = "<html><body><p>" + hostname + "</p></body></html>"
				elif path == '/cpu-name':
					response_header = self.gen_headers(200)
					response_content = "<html><body><p>" + cpu_info + "</p></body></html>"
				elif '/load' in path:

					cpu_load = str(round(sum(os.getloadavg()) / len(os.getloadavg()),3))
					response_content = "<html><body><p>" + cpu_load + "</p></body></html>"
					
					refresh = path.split('?')

					if len(refresh) > 1:
						#refresh = refresh[1]

						if 'refresh=' in refresh:

							self.time = refresh.split("=")[1]
							logging.debug("refresh set to {0}s".format(self.time))
							self.host = data.split('\n')
							response_header = self.gen_headers(1)
					else:
						response_header = self.gen_headers(200)
					

				else:
					response_header = self.gen_headers(404)
					logging.debug("NOT FOUND")

			else:
				response_header = self.gen_headers(405)
				logging.debug("BAD request")

			
			conn.send((response_header + response_content).encode('utf-8'))
			logging.debug("Closing connection with client")
			conn.close()
	
	
	
					
def shutdown(sig, dummy):
    """ This function shuts down the server. It's triggered
    by SIGINT signal """
    s.shutdown_server() #shut down the server
    import sys
    sys.exit(1)

#main function
if __name__ == '__main__':
	# shut down on ctrl+c
	signal.signal(signal.SIGINT, shutdown)
	logging.basicConfig(level=logging.DEBUG)
	#exit(1)
	s = Server(8000)
	s.start_server()
