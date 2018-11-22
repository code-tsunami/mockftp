/*******************************************************************************
 * Sonam Kindy
 * Description: server-side (ftserver)
 * Last Modified: 11/18/18
 * Ref: https://beej.us/guide/bgnet/html/multi/clientserver.html#simpleserver
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/stat.h>
#include <dirent.h>

#define MAX_CONNECTIONS	1		// max connections to facilitate; fixed num for this prog: 1
#define MAX_CHARS		1024	// max number of chars to send at a time via socket
#define MAX_FILES		1024	// max number of files to expect on server cwd
#define MAX_ARGS		10 		// fixed num for this prog: 5 

int caughtSIGINT = 0; // global var for whether SIGINT caught
char clientHostName[MAX_CHARS]; // global var for client host name

/*******************************************************************************
 * void error(const char* msg) 
 * 
 * Description: Error function used for reporting issues 
 * params: 		msg (const char*)
 * ret: 		N/A
 ******************************************************************************/
void error(const char* msg) 
{
	perror(msg); 
	exit(1); 
}

/*******************************************************************************
 * void catchSignal(int signo)
 * 
 * Description: Catch and handle signal (in this case, only SIGINT or ^C)
 * params: 		signo (int)
 * ret: 		N/A
 ******************************************************************************/
void catchSignal(int signo)
{
	char* msg = "\n\nServer shut down.\n"; // msg to output if SIGINT caught

	switch(signo) {
		case SIGINT:
			write(STDOUT_FILENO, msg, strlen(msg));
			caughtSIGINT = 1;
			break;
		default:
			break;
	}
}

/*******************************************************************************
 * void registerSigHandler(const int SIG, const int FLAG)
 * 
 * Description: Register specified signal with catchSignal signal handler 
 *				and specified flag.
 * params: 		SIGNO (const int)
 *				FLAG (const int)
 * ret: 		N/A
 ******************************************************************************/
void registerSigHandler(const int SIGNO, const int FLAG)
{
	struct sigaction sa = {{0}};
	sa.sa_handler = catchSignal;
	sigfillset(&sa.sa_mask); // block/delay all signals while this mask is in place
	sa.sa_flags = FLAG;
	sigaction(SIGNO, &sa, NULL);
}

/*******************************************************************************
 * int setupControlSocket(char* portNumberStr)
 * 
 * Description: Set up socket to be used for TCP control connection between server 
 *				and client.
 * params: 		portNumberStr (char*)
 * ret: 		controlSocketFD (int)
 ******************************************************************************/
int setupControlSocket(char* portNumberStr)
{
	// Set up the address struct for this process (the server)
	struct sockaddr_in serverAddress;
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	int portNumber = atoi(portNumberStr); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Allow any address to connect to this process

	// Set up the socket
	int controlSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (controlSocketFD < 0) 
		error("ERROR with socket()");

	// Enable the socket to begin listening; connect socket to port (if error output error)
	if (bind(controlSocketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
		error("ERROR with bind()");

	// Flip the socket on - it can now receive up to MAX_CONNECTIONS connections
	listen(controlSocketFD, MAX_CONNECTIONS); 

	return controlSocketFD;
}

/*******************************************************************************
 * int acceptConnection(int socketFD)
 * 
 * Description: Accept incoming connection from client
 * Ref:			For specifying host name: https://beej.us/guide/bgnet/html/multi/getnameinfoman.html
 * params: 		socketFD (int)
 * ret: 		dataSocketConnectionFD (int)
 ******************************************************************************/
int acceptConnection(int socketFD)
{
	struct sockaddr_in clientAddress;
	socklen_t sizeOfClientInfo = sizeof(clientAddress); // Get size of the addr for client that connecting
	// Accept a connection, blocking if one is not available until one connects
	int dataSocketConnectionFD = accept(socketFD, (struct sockaddr*)&clientAddress, &sizeOfClientInfo);
	if(dataSocketConnectionFD == -1)
		return -1;

	char service[20];
	getnameinfo((struct sockaddr*)&clientAddress, sizeof(clientAddress), clientHostName, sizeof(clientHostName), service, sizeof(service), 0);
	printf("Connection from %s\n\n", clientHostName);
	
	return dataSocketConnectionFD;
}

/*******************************************************************************
 * char** getDirListing(char* dirname)
 * 
 * Description: Get all the names of files in specified dir. Return the generated 
 *				array of strings.
 * params: 		dirname (char*)
 * ret: 		filenamesInDir (char**)
 ******************************************************************************/
char** getDirListing(char* dirname)
{
	char** filenamesInDir = (char**)malloc(MAX_FILES * sizeof(char*));
	for(int i = 0 ; i < MAX_FILES; i++)
		filenamesInDir[i] = NULL;

	char* filename = malloc(MAX_CHARS * sizeof(char));

	DIR* dirToCheck = opendir(dirname);
	if(dirToCheck == NULL) 
		error("getDirListing(): opendir() failed");
	
	struct dirent* fileInDir;

	int i = 0;
	// while there remains a file (or dir) to read in the given dir
	while((fileInDir = readdir(dirToCheck)) != NULL) {
		// verify that entry is regular file
		if(fileInDir->d_type == DT_REG) {
			memset(filename, '\0', MAX_CHARS); // clear out filename

			// write filename to filename var
			int res = sprintf(filename, "%s", fileInDir->d_name); 
			if(res < 0) 
				error("getDirListing(): sprintf() failed");
			
			// write filename to filenamesInDir str arr
			filenamesInDir[i] = (char*)malloc(MAX_CHARS * sizeof(char));
			memset(filenamesInDir[i], '\0', MAX_CHARS);
			strcpy(filenamesInDir[i], filename);

			i++;
		}
	}

	free(filename); /* free the memory dynamically alloc. via malloc() */
	closedir(dirToCheck); /* close the directory we opened */

	return filenamesInDir;
} 


/*******************************************************************************
 * int setUpSocket(char* hostname, char* portNumStr)
 * 
 * Description: Set up socket (in this case, data socket).
 * params: 		hostname (char*)
 * ret: 		portNumStr (char**)
 ******************************************************************************/
int setUpSocket(char* hostname, char* portNumStr)
{
	/* Convert to portNum to an int from a str */
	int portNum = atoi(portNumStr);
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;

	/* Set up the server address struct */
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); /* Clear out the address struct */
	serverHostInfo = gethostbyname(hostname); /* Convert the machine name into a special form of address */
	/* if host doesn't exist, output error and exit prog */
	if(serverHostInfo == NULL)
		error("SERVER: ERROR, no such host");
	
	serverAddress.sin_family = AF_INET; /* Create a network-capable socket */
	serverAddress.sin_port = htons(portNum); /* Store the port number */
	/* Copy in the address */
	memcpy(	(char*)&serverAddress.sin_addr.s_addr, 
			(char*)serverHostInfo->h_addr, 
			serverHostInfo->h_length); 

	/* Set up the socket; if error, output error and exit prog */
	int socketFD = socket(AF_INET, SOCK_STREAM, 0); /* Create the socket */
	if (socketFD < 0) 
		error("SERVER: ERROR opening socket");
	
	/* Connect to server; if error, output error and exit prog */
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) /* Connect socket to address */
		error("SERVER: ERROR connecting");

	return socketFD;
}

/*******************************************************************************
 * void receiveMsg(int socketFD, char* receivedMsg)
 * 
 * Description: Receive message from host on other end via specified socket.
 * params: 		socketFD (int)
 *				receivedMsg (char*)
 * ret: 		N/A
 ******************************************************************************/
void receiveMsg(int socketFD, char* receivedMsg)
{
	// To contain message received from server; no new-line should be part of msg 
	char receivedBuffer[MAX_CHARS]; 
	memset(receivedBuffer, '\0', sizeof(receivedBuffer)); // Clear out the receivedBuffer again for reuse
	// Get return message from host on other end 
	int charsRead = recv(socketFD, receivedBuffer, sizeof(receivedBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) 
		error("SERVER: ERROR reading from socket");
	strcpy(receivedMsg, receivedBuffer); // Copy received buffer str to received msg str
}

/*******************************************************************************
 * void sendMsg(int socketFD, const char* msgToSend)
 * 
 * Description: Send msg through socket channel to host on other end.
 * params: 		socketFD (int)
 *				msgToSend (const char*)
 * ret: 		N/A
 ******************************************************************************/
void sendMsg(int socketFD, const char* msgToSend)
{
	int charsWritten;
	do {
		charsWritten = send(socketFD, msgToSend, strlen(msgToSend), 0); 
		if (charsWritten < 0) 
			error("SERVER: ERROR writing to socket");
	} while(charsWritten < strlen(msgToSend));
}

/*******************************************************************************
 * void sendMsg(int socketFD, const char* msgToSend)
 * 
 * Description: Create and return an args str arr of each space-delimited value 
 *				in given str.
 * params: 		str (char*)
 * ret: 		args (char**)
 ******************************************************************************/
char** getArgs(char* str)
{
	char** args = (char**)malloc(MAX_ARGS * sizeof(char*));
	for(int i = 0; i < MAX_ARGS; i++)
		args[i] = NULL;

	const char delim[2] = " ";
	char* token = strtok(str, delim);
	int i = 0;
	while(token != NULL) {
		args[i] = (char*)malloc(MAX_CHARS * sizeof(char));
		memset(args[i], '\0', MAX_CHARS); // make sure the current arg null terminates
		strcpy(args[i], token); // copy token str to args at i

		token = strtok(NULL, delim); // get next token
		
		i++; // next available index in args str arr
	}

	return args;
}

/*******************************************************************************
 * void sendMsg(int socketFD, const char* msgToSend)
 * 
 * Description: Verify if given file exists. 
 * Source:		https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform
 * params: 		filename (char*)
 * ret: 		1 if file exists; else 0
 ******************************************************************************/
int fileExists(char*  filename)
{
	struct stat buffer;   
	return(stat(filename, &buffer) == 0);
}

/*******************************************************************************
 * void sendFilenamesInDir(char** filenames, char* dataPortStr)
 * 
 * Description: Init a socket at the given port, and for each str in the given str 
 *				arr, and write each str to it.
 * params: 		filenames (char**)
 *				dataPortStr (char*)
 * ret: 		N/A
 ******************************************************************************/
void sendFilenamesInDir(char** filenames, char* dataPortStr)
{
	int dataSocketFD = setUpSocket(clientHostName, dataPortStr);
	
	// for each filename send to other host via data socket
	for(int i = 0; i < MAX_FILES && filenames[i] != NULL; i++) {
		// write curr filename to data socket
		sendMsg(dataSocketFD, filenames[i]);
		// write new line to data socket so output on client-side formatted nicely
		if(filenames[i + 1] != NULL)
			sendMsg(dataSocketFD, "\n");
	}
	sendMsg(dataSocketFD, "__EOF__"); // done; send __EOF__ str to signify end

	close(dataSocketFD); // done sending information via data socket
}

/*******************************************************************************
 * void sendFileContents(char* filename, char* dataPortStr)
 * 
 * Description: Init a socket at the given port, and for the given file, and write 
 *				each line to the socket.
 * params: 		filename (char*)
 *				dataPortStr (char*)
 * ret: 		N/A
 ******************************************************************************/
void sendFileContents(char* filename, char* dataPortStr)
{
	int dataSocketFD = setUpSocket(clientHostName, dataPortStr);

	char line[MAX_CHARS];
	char* buffer;
	FILE* stream = fopen(filename, "r");
	// while EOF not reached read line of file and send to client-side via data socket
	while((buffer = fgets(line, MAX_CHARS, stream)) != NULL)
		sendMsg(dataSocketFD, buffer);
	sendMsg(dataSocketFD, "__EOF__"); // done; send __EOF__ str to signify end

	close(dataSocketFD); // done sending information via data socket
}

/*******************************************************************************
 * void freeStrArr(char** arr)
 * 
 * Description: Free memory allocated to str arr for every item in arr until NULL
 *				i.e. end of str arr encountered.
 * params: 		strArr (char**)
 * ret: 		N/A
 ******************************************************************************/
void freeStrArr(char** strArr)
{
	for(int i = 0; strArr[i] != NULL; i++)
		free(strArr[i]);
	free(strArr);
}

/*******************************************************************************
 * void initFTPConnection(int controlSocketFD)
 * 
 * Description: Send and receive information between server and client.
 * params: 		controlSocketFD (int)
 * ret: 		N/A
 ******************************************************************************/
void initFTPConnection(int controlSocketFD)
{
	char receivedBuffer[MAX_CHARS];
	memset(receivedBuffer, '\0', sizeof(receivedBuffer));

	char sendBuffer[MAX_CHARS];
	memset(sendBuffer, '\0', sizeof(sendBuffer));

	// read data from the control socket; get command-line input
	receiveMsg(controlSocketFD, receivedBuffer);

	// parse arguments
	char** args = getArgs(receivedBuffer);

	char host[MAX_CHARS];
	memset(host, '\0', sizeof(host));
	memcpy(host, args[0], sizeof(host) - 1);

	char controlPort[6];
	memset(controlPort, '\0', sizeof(controlPort));
	memcpy(controlPort, args[1], sizeof(controlPort) - 1);
	char dataPort[6];
	memset(dataPort, '\0', sizeof(dataPort));

	char command[3];
	memset(command, '\0', sizeof(command));
	memcpy(command, args[2], sizeof(command) - 1);

	char controlMsg[MAX_CHARS];
	memset(controlMsg, '\0', sizeof(controlMsg));
	strcpy(controlMsg, "OK"); // default msg

	// if l flag, listing of files in cwd was requested
	if(strcmp(command, "-l") == 0) {
		sprintf(dataPort, "%s", args[3]);
		printf("List directory requested on port %s\n", dataPort);
		printf("Sending directory contents to %s:%s\n\n", clientHostName, dataPort);
		
		char** filenames = getDirListing("."); // get filenames in cwd
		sendMsg(controlSocketFD, controlMsg); // send OK msg
		// send names of filenames in directory via data socket at given port
		sendFilenamesInDir(filenames, dataPort);
		freeStrArr(filenames); // free memory allocated to filenames
	} 
	// if g flag, specified file contents in cwd was requested
	else if(strcmp(command, "-g") == 0) {
		sprintf(dataPort, "%s", args[4]);
		
		char filename[100];
		memset(filename, '\0', sizeof(filename));
		memcpy(filename, args[3], sizeof(filename) - 1);

		printf("File \"%s\" requested on port %s\n", filename, dataPort);

		if(fileExists(filename)) {
			printf("Sending \"%s\" to %s:%s\n\n", filename, clientHostName, dataPort);
			// send file contents via data socket at given port
			sendMsg(controlSocketFD, controlMsg); // send OK/failure msg to client-side via control socket
			sendFileContents(filename, dataPort);
		} else {
			printf("File \"%s\" not found; sending error msg to %s:%s\n\n", filename, clientHostName, controlPort);
			sprintf(controlMsg, "File \"%s\" not found", filename); // update control msg to failure since file not found
			sendMsg(controlSocketFD, controlMsg); // send OK/failure msg to client-side via control socket
		}
	}
	// otherwise command not recognized (won't occur since input validation in place on client-side); just defensive programming
	else {
		printf("Command \"%s\" is invalid; sending error msg to %s:%s", command, clientHostName, controlPort);
		strcpy(controlMsg, "Invalid command");  // update control msg to failure since command invalid
		sendMsg(controlSocketFD, controlMsg); // send OK/failure msg to client-side via control socket
	}

	freeStrArr(args); // free memory allocated to args
}

/*******************************************************************************
 * void waitForConnection(int socketFD, int controlPortNum)
 * 
 * Description: Wait for incoming connection from client. Handle it as needed.
 * params: 		socketFD (int)
 *				controlPortNum (int)
 * ret: 		N/A
 ******************************************************************************/
void waitForConnection(int socketFD, int controlPortNum)
{
	while(!caughtSIGINT) {
		printf("Server open for connection on port %d...\n\n", controlPortNum);
		int clientFD = acceptConnection(socketFD);
		// as long as no accept() failure, init FTP connection and handle req
		if(clientFD != -1) 
			initFTPConnection(clientFD);
		close(clientFD);
	}
}

/*******************************************************************************
 * int main(int argc, char* argv[])
 * 
 * Description: Validate command-line args, register signal handler for SIGINT and
 * 				open control port socket for connection from client. Close when 
 *				SIGINT caught.
 * params: 		argc (int)
 *				argv (char**)
 * ret: 		N/A
 ******************************************************************************/
int main(int argc, char* argv[])
{
	// Check usage & args
	if (argc < 2) {
		fprintf(stderr,"USAGE: %s port\n", argv[0]);
		exit(1); 
	}

	registerSigHandler(SIGINT, 0); // register sig handler for SIGINT (^C)

	int controlSocketFD = setupControlSocket(argv[1]); // set up control socket

	// wait for connections from other host; accept connection and init FTP connection
	waitForConnection(controlSocketFD, atoi(argv[1])); 

	// free memory
	close(controlSocketFD);

	return 0; 
}
