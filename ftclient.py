#!/usr/bin/env python3

# Sonam Kindy
# Description: client-side (ftclient)
# Created: 11/17/18
# Last Modified: 11/18/18
# Ref: Python socket documentation

import socket
import argparse
import os

NUM_CHARS_MSG = 1024

def main(args):
	"""
	Run client and connect to server via control port
	Init data socket via specified data port
	Init FTP connection and exchange data/information between hosts

	Keyword arguments:
	args -- command-line args (Namespace)
	"""
	control_socket = connect_to_host(args.server_host, args.server_control_port)
	print('Client connected to server control socket...')
	data_socket = init_socket(args.data_port)
	print('Client data socket listening...\n')
	init_FTP_connection(control_socket, data_socket, args)

def connect_to_host(host, port):
	"""
	Init socket; connect to host at specified port

	Keyword arguments:
	host -- hostname (str)
	port -- port number (int)
	"""
	control_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	control_socket.connect((host, port))
	return control_socket

def init_FTP_connection(control_socket, data_socket, args):
	"""
	Init FTP connection given sockets and command-line args
	Get requests from other host sent via control socket
	Respond to requests via data socket
	Send any error message re: requests via control socket

	Keyword arguments:
	control_socket -- socket opened on server-side at control port (socket obj)
	data_socket -- socket opened on client-side at data port (socket obj)
	"""
	# determine what command is being requested
	file_transfer = False
	if args.filename:
		print("Client requesting {}\n".format(args.filename))
		file_transfer = True
	else:
		print("Client requesting file listing\n")

	commandline_str = get_commandline_str(args) # format command-line str into consistent and specific format
	control_socket.send(commandline_str.encode('utf-8')) # send command line str

	# check for error msg file not found/invalid command (though the latter won't ever occur due to client-side validation)
	error_msg = (control_socket.recv(NUM_CHARS_MSG)).decode('utf-8')
	if error_msg != "OK":
		error(error_msg, args, control_socket)

	# accept incoming connection from server-side so comm. can happen via data port
	data_connection_socket, addr = data_socket.accept()

	# handle file transfer
	if file_transfer:
		print("Receiving \"{}\" from {}:{}...\n".format(args.filename, args.server_host, args.data_port))
		success = get_file(args.filename, data_connection_socket)
		if success:
			print("File \"{}\" transfer complete".format(args.filename))
		else:
			print("File \"{}\" not transferred; already exists in {}".format(args.filename, os.getcwd()))
	# handle dir file listing
	else:
		print("Receiving directory structure from {}:{}...\n".format(args.server_host, args.data_port))
		get_dir_file_listing(data_connection_socket)

	data_connection_socket.close() # close data connection socket
	control_socket.close() # close control socket

def error(error_msg, args, control_socket):
	"""
	In the event of server-side error sent via control socket, print error message, close control socket 
	and exit with error status code 1 (indicating an error occurred)

	Keyword arguments:
	error_msg -- error message sent from server-side via control socket (str)
	control_socket -- socket opened on server-side at control port (socket obj)
	"""
	print("{}:{} says: {}".format(args.server_host, args.server_control_port, error_msg))
	control_socket.close()
	exit(1)

def get_commandline_str(args):
	"""
	Use args to form a predictable/consistently formatted str and return it

	Keyword arguments:
	args -- command-line args (Namespace)
	"""
	commandline_str = "{} {} {}".format(args.server_host, str(args.server_control_port), str(args.data_port))
	data_port_index = commandline_str.find(str(args.data_port))
	if args.filename:
		commandline_str = "{} -g {} {}".format(commandline_str[:data_port_index - 1], args.filename, commandline_str[data_port_index:])
	else:
		commandline_str = "{} -l {}".format(commandline_str[:data_port_index - 1], commandline_str[data_port_index:])
	return commandline_str

def get_file(filename, data_socket):
	"""
	Get file contents of specified file in other host's cwd via data socket from other host 

	Keyword arguments:
	data_socket -- socket opened on client-side at data port (socket obj)
	"""
	# if file already exists don't overwrite its contents on client-side
	if os.path.exists(filename):
		return False
	# otherwise open file and write the contents sent from other host via data socket
	f = open(filename, "w")
	file_buffer = (data_socket.recv(NUM_CHARS_MSG)).decode('utf-8')
	should_continue = True
	while should_continue:
		file_stream = file_buffer.replace("__EOF__", ""); # strip off __EOF__
		f.write(file_stream)
		# get more contents from the file if not EOF
		if "__EOF__" not in file_buffer:
			file_buffer = (data_socket.recv(NUM_CHARS_MSG)).decode('utf-8')
		else:
			should_continue = False
	return True # file written ("transferred")

def get_dir_file_listing(data_socket):
	"""
	Get file listing of cwd of other host via data socket from host on other end

	Keyword arguments:
	data_socket -- socket opened on client-side at data port (socket obj)
	"""
	filenames = (data_socket.recv(NUM_CHARS_MSG)).decode('utf-8')
	should_continue = True
	# while filenames not an empty string
	while should_continue:
		out_stream = filenames.replace("__EOF__", ""); # strip off __EOF__
		print(out_stream)
		# get more filenames if not EOF
		if "__EOF__" not in filenames:
			filenames = (data_socket.recv(NUM_CHARS_MSG)).decode('utf-8')
		else:
			should_continue = False

def init_socket(port):
	"""
	Init socket at specified port. Listen for connections. 
	Return socket obj.
	
	Keyword arguments:
	port -- port at whicch to init socket (int)
	"""
	connection_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	connection_socket.bind(('', port))
	connection_socket.listen(1)
	return connection_socket

def validate_port(val):
	"""
	Validate server port command-line arg. 
	That is, verify it is an int and valid, i.e. in the range [1024, 65535].

	Keyword arguments:
	val -- value to validate is in the required range and can be cast to an int (str)
	"""
	MAX_PORT_NUM = 65535
	MIN_PORT_NUM = 1024
	try:
		int_val = int(val)
	except:
		raise argparse.ArgumentTypeError("\"{}\" is not a valid integer".format(val))
	if int_val < MIN_PORT_NUM or int_val > MAX_PORT_NUM:
		 raise argparse.ArgumentTypeError("{} must be [{}, {}]".format(val, str(MIN_PORT_NUM), str(MAX_PORT_NUM)))
	return int_val

def get_args():
	"""
	Define positional arguments and required group of arguments
	Return args Namespace if valid/required arguments given

	NOTE: ArgumentParser doesn't require specific order of parameters when using groups so if entire command-line str needed, 
	should be reformatted appropriately
	"""
	# use argument parser to parse and designate required args of the server host and port to use
	parser = argparse.ArgumentParser(description='Set up and manage client-side for TCP control and data port connection')
	# server host required
	parser.add_argument("server_host", type=str, help="designate the host to use for the server",
						action="store")
	# server port required
	parser.add_argument("server_control_port", type=validate_port, help="designate the port used to set up the control port on the server",
						action="store")
	# either -g FILENAME or -l are required
	group = parser.add_mutually_exclusive_group(required=True)
	group.add_argument("-g", dest="filename", type=str, help="designate the file to get from the server",
						action="store")
	group.add_argument("-l", help="get directory file listing from the server",
						action="store_true")
	# data connection port required
	parser.add_argument("data_port", type=validate_port, help="designate the port to use for the data port; must be [1024, 65535]",
						action="store")
	return parser.parse_args()

# Execute main method upon running script directly
if __name__ == '__main__':
	main(get_args()) # call main with command-line args
