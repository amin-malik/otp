#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define ALPHABET 27
#define BUF_SIZE 500

void error(const char *msg) {
	perror(msg);
	exit(1);
}

int transformCharToInt(char inputChar) {
	if (inputChar == ' ') {
		return ALPHABET -1;
	}
	if (inputChar >= 'A' && inputChar <= 'Z') {
		return inputChar - 'A';
	}
	return -1;
}

char* decrypt(char *key, char *text, int len) {
	int i;
	int keyVal;
	int charVal;
	char *plain = (char*) malloc(len) + 1;
	memset(plain, 0, len);

	for (i = 0; i < len; i++) {
		charVal = transformCharToInt(text[i]);
		keyVal = transformCharToInt(key[i]);
		int result = (charVal - keyVal);
		if (result < 0) {
			result += ALPHABET;
		}
		if (result == ALPHABET -1) {
			plain[i] = ' ';
		} else {
			plain[i] = 'A' + result;
		}
	}
	return plain;
}

void setupAddressStruct(struct sockaddr_in *address, int portNumber) {

	// Clear out the address struct
	memset((char*) address, '\0', sizeof(*address));

	// The address should be network capable
	address->sin_family = AF_INET;
	// Store the port number
	address->sin_port = htons(portNumber);
	// Allow a client at any address to connect to this server
	address->sin_addr.s_addr = INADDR_ANY;
}

// Read a message in chunks until it's all been received
char* readMessage(int fd, int l) {
	char *message = (char*) malloc(l + 1);
	message[l] = '\0';
	int read = 0;
	int blockSize = 512;
	while (read < l) {
		if (l - read < blockSize) {
			blockSize = l - read;
		}
		int c = recv(fd, message, blockSize, 0);
		if (c <= 0) {
			break;
		}
		read += c;
	}

	return message;
}

// Write a message in chunks until it's all sent
void writeMessage(int fd, char *message, int l) {
	int blockSize = 512;
	int written = 0;
	while(written < l) {
		if(l - written < blockSize) {
			blockSize = l - written;
		}
		int c = send(fd, message + written, blockSize, 0);
		if(c <= 0) {
			break;
		}
		else {
			written += c;
		}
	}
}

int main(int argc, char *argv[]) {
	int connectionSocket;
	char buffer[256];
	struct sockaddr_in serverAddress, clientAddress;
	socklen_t sizeOfClientInfo = sizeof(clientAddress);

	// Check usage & args
	if (argc < 2) {
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);
	}

	// Create the socket that will listen for connections
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket < 0) {
		error("ERROR opening socket");
	}

	// Set up the address struct for the server socket
	setupAddressStruct(&serverAddress, atoi(argv[1]));

	// Associate the socket to the port
	if (bind(listenSocket, (struct sockaddr*) &serverAddress,
			sizeof(serverAddress)) < 0) {
		error("ERROR on binding");
	}

	// Start listening for connetions. Allow up to 5 connections to queue up
	listen(listenSocket, 5);

	// Accept a connection, blocking if one is not available until one connects
	while (1) {
		int req;
		// Accept the connection request which creates a connection socket
		connectionSocket = accept(listenSocket,
				(struct sockaddr*) &clientAddress, &sizeOfClientInfo);
		if (connectionSocket < 0) {
			error("ERROR on accept");
		}

		printf("SERVER: Connected to client running at host %d port %d\n",
				ntohs(clientAddress.sin_addr.s_addr),
				ntohs(clientAddress.sin_port));

		// Clear the buffer 
		memset(buffer, '\0', 256);
		
		// Receive an initial connection to identify the decryption client
		recv(connectionSocket, &req, sizeof(char), 0);
		if (req == 'D') {
			recv(connectionSocket, &req, sizeof(int), 0);
			int l = req;
			char *key = readMessage(connectionSocket, l);
			char *cipher = readMessage(connectionSocket, l);
			char *decryptedText = decrypt(cipher, key, l);
			// Send a Success message back to the client
			writeMessage(connectionSocket, decryptedText, l);
		} else {
			fprintf(stderr, "Unable to verify the decryption client's identity\n. Connection refused!!\n");
			exit(2);
		}
		// Close connection socket
		close(connectionSocket);
	}
	// Close the listening socket
	close(listenSocket);

	return 0;
}
