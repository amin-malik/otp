#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()

/**
 * Client code
 * 1. Create a socket and connect to the server specified in the command arugments.
 * 2. Prompt the user for input and send that input as a message to the server.
 * 3. Print the message received from the server and exit the program.
 */

// Error function used for reporting issues
void error(const char *msg) {
	perror(msg);
	exit(0);
}

bool ValidateInput(char *input) {
	while (*input) {
		if (*input == '\n' || *input == ' ') {
			return true;
		} else {
			if (*input > 'Z' || *input < 'A') {
				return false;
			}
		}
		input++;
	}
	return true;
}

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

// Set up the address struct
void setupAddressStruct(struct sockaddr_in *address, int portNumber,
		char *hostname) {

	// Clear out the address struct
	memset((char*) address, '\0', sizeof(*address));

	// The address should be network capable
	address->sin_family = AF_INET;
	// Store the port number
	address->sin_port = htons(portNumber);

	// Get the DNS entry for this host name
	struct hostent *hostInfo = gethostbyname(hostname);
	if (hostInfo == NULL) {
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(0);
	}
	// Copy the first IP address from the DNS entry to sin_addr.s_addr
	memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0],
			hostInfo->h_length);
}

char * readFile(char *filename) {
	FILE *fp = fopen(filename, "rb");
	if (! fp) {
			fprintf(stderr, "failed to open %s\n", filename);
			exit(0);
	}
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp) -1;
	fseek(fp, 0, SEEK_SET);
	char *data = malloc(size + 1);
	data[size] = '\0';
	fread(data, 1, size, fp);
	return data;
}

int main(int argc, char *argv[]) {
	int socketFD, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	int key_len, data_len;

	char *key_pointer, *data_pointer;

	// Check usage & args
	if (argc < 4) {
		fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
		exit(0);
	}

	data_pointer = readFile(argv[1]);
	key_pointer = readFile(argv[2]);
	key_len = strlen(key_pointer);
	data_len = strlen(data_pointer);

	if (key_len < data_len) {
		fprintf(stderr,
				"Error: The given key '%s' cannot be shorter than the given input text\n",
				argv[2]);
		exit(1);
	}

	// Create a socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD < 0) {
		error("CLIENT: ERROR opening socket");
	}

	// Set up the server address struct
	setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*) &serverAddress,
			sizeof(serverAddress)) < 0) {
		error("CLIENT: ERROR connecting");
	}

	// Send message to server
	// Write to the server
	char signature = 'E';
	send(socketFD, &signature, 1, 0);
	send(socketFD, &data_len, sizeof(int), 0);

	charsWritten = send(socketFD, data_pointer, data_len, 0);
	if (charsWritten < 0) {
		error("CLIENT: ERROR writing to socket");
	}

	charsWritten = send(socketFD, key_pointer, key_len, 0);
	if (charsWritten <= 0) {
		error("CLIENT: ERROR unable to verify client identity");
	}

	// Get return message from server
	// Clear out the buffer again for reuse
	memset(data_pointer, '\0', data_len);
	// Read data from the socket, leaving \0 at end
	charsRead = recv(socketFD, data_pointer, data_len, 0);
	if (charsRead < 0) {
		error("CLIENT: ERROR reading response from socket");
	}
	printf("%s\n", data_pointer);

	// Close the socket
	close(socketFD);
	return 0;
}

