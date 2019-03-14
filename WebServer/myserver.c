#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BACKLOG 80

// for status OK, NOT FOUND
enum Status {
	OK, NOT_FOUND
};

// file descriptor of server, client socket and data file
static int servfd, clntfd, datafd;

// forward declaration of functions
void CleanExit(int);
void OpenServer(int);
void AcceptClient();
void HandleRequest();
void ParseMessage(char*, char*, char*, char*);
void SendData(char*, char*);
void ContentType(char*, char*);
void ContentLength(char*, char*);
enum Status Protocol(char*, char*);

int main(int argc, char* argv[])
{
	// error handling
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [port number]\n", argv[0]);
        exit(1);
    }

	// if program terminated, CleanExit will be called.
    signal(SIGINT, CleanExit);
    signal(SIGTERM, CleanExit);

	// open server.
    OpenServer(atoi(argv[1]));

	// infinite loop for communication.
    for (;;) {
		// accept client
		AcceptClient();

		// receive request message and send data.
        HandleRequest();

		// close client socket
		close(clntfd);
    }

	// close server socket and exit.
    close(servfd);
    return 0;
}

/**
 * Close all socket and data file.
 * PARAM
 * @signo error code
 */
void CleanExit(int signo)
{
    if (servfd != 0)
        close(servfd);
    if (clntfd != 0)
        close(clntfd);
    if (datafd != 0)
        close(datafd);

    exit(1);
}

/**
 * Open server
 * PARAM
 * @port port number
 */
void OpenServer(int port)
{
    struct sockaddr_in serv_addr;

    printf("Server opening...\n");

	// open server socket.
    if ((servfd = socket(PF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket error");
        CleanExit(errno);
    }
    
	// initialize server socket address object
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
    serv_addr.sin_port = htons(port);

	// bind server socket
    if (bind(servfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind error");
        CleanExit(errno);
    }

	// listen server socket
    if (listen(servfd, BACKLOG) == -1) {
        perror("listen error");
        CleanExit(errno);
    }

    printf("Server ready!\n");
}

/**
 * Accept client
 */
void AcceptClient()
{
	struct sockaddr_in clnt_addr;
	int clnt_addr_size = sizeof(clnt_addr);

	printf("Wait for accepting...\n");

	// accept client
	if ((clntfd = accept(servfd, (struct sockaddr*)&clnt_addr, &clnt_addr_size)) == -1) {
		perror("accept error");
		CleanExit(errno);
	}

	printf("Client accepted!\n");
}

/**
 * receive request and send response
 */
void HandleRequest()
{
    char request[10000];
    char method[16];
    char file_type[32];
    char file_name[64];
	char buf[1024];

	// initialize buffers with 0
    memset(request, '\0', 10000);
    memset(method, '\0', 16);
    memset(file_type, '\0', 32);
    memset(file_name, '\0', 64);

	// wait and receive request
	printf("\nWaiting for message\n");
	read(clntfd, request, 9999);
	printf("\nMessage received\n\n");

	// print request message
	printf("===== Request =====\n");
	printf("%s", request);
	printf("===================\n\n");

	// parse request message
	ParseMessage(request, method, file_name, file_type);

	// print essential information in this app.
    printf("Message method: %s\n", method);
    printf("File Name: %s\n", file_name);
    printf("Content Type: %s\n", file_type);

	// send data.
    SendData(file_type, file_name);
}

/**
 * Parse request message
 * PARAM
 * @request full request message given by client
 * @method buffer for method like GET/POST
 * @file_name buffer for file name
 * @file_t buffer for file type
 */
void ParseMessage(char* request, char* method, char* file_name, char* file_t)
{
	char* ptr;
	int notfound = 0;

	// parse method
	sprintf(method, "%s", strtok(request, " /"));
	
	// parse file name.
	if ((ptr = strtok(NULL, " /")) == NULL) {
		sprintf(file_name, "notfound.html");
		notfound = 1;
	}
	else {
		sprintf(file_name, "%s", ptr);
	}

	// parse file type
	if (notfound == 1 || (ptr = strstr(file_name, ".")) == NULL) {
		sprintf(file_t, "html");
		notfound = 1;
	}
	else {
		sprintf(file_t, "%s", (ptr + 1));
	}
}

/**
 * Send data to client
 * PARAM
 * @file_type file type
 * @file_name file name
 */
void SendData(char* file_type, char* file_name)
{
    char protocol[64];
    char server[] = "Server: Linux Web Server \r\n";
    char content_len[64];
    char content_type[64];
    char file[128];
    char buf[1024];
	int n;

	// initialize buffer with 0
    memset(protocol, '\0', 64);
    memset(content_len, '\0', 64);
    memset(content_type, '\0', 64);

	// correct file path
	sprintf(file, "static/%s", file_name);

	// set protocol buffer. this will contain first line of response.
	if (Protocol(file, protocol) == NOT_FOUND) {
		sprintf(file, "static/notfound.html");
		sprintf(file_type, "html");
	}
	// set content type buffer.
    ContentType(file_type, content_type);
	// set content length buffer
    ContentLength(file, content_len);

	// write header of response to client socket
    write(clntfd, protocol, strlen(protocol));
   	write(clntfd, server, strlen(server));
    write(clntfd, content_len, strlen(content_len));
	// for downloading pdf file(Chrome browser only)
	if (strcmp(file_type, "pdf") == 0) {
		write(clntfd, "Content-Disposition: attachment; filename=\"network.pdf\"\r\n",
				strlen("Content-Disposition: attachment; filename=\"network.pdf\"\r\n"));
		write(clntfd, "Cache-Control: no-cache, no-store\r\n",
				strlen("Cache-Control: no-cache, no-store\r\n"));
		write(clntfd, "Content-Transfer-Encoding: binary\r\n",
				strlen("Content-Transfer-Encoding: binary\r\n"));
	}
   	write(clntfd, content_type, strlen(content_type));

	// print file name to send
	printf("File for sending: %s\n", file);
	
	// open data file to send
	if ((datafd = open(file, O_RDONLY)) == -1) {
		perror("open error");
		CleanExit(errno);
	}

	// read data file and write to socket
    while ((n = read(datafd, buf, 1024)) > 0) {
        write(clntfd, buf, n);
    }

	printf("\nSending complete.\n\n");

	// close data file.
    close(datafd);
	datafd = 0;
}

/**
 * parse content type and write to buffer
 * PARAM
 * @content_type content type
 * @buffer content type buffer
 */
void ContentType(char* content_type, char* buffer)
{
    if (strcmp(content_type, "mp3") == 0) {
        sprintf(buffer, "Content-Type: audio/mpeg\r\n\r\n");
    }
    else if (strcmp(content_type, "html") == 0
            || strcmp(content_type, "plain") == 0) {
        sprintf(buffer, "Content-Type: text/%s\r\n\r\n", content_type);
    }
    else if (strcmp(content_type, "gif") == 0
            || strcmp(content_type, "png") == 0
            || strcmp(content_type, "bmp") == 0) {
        sprintf(buffer, "Content-Type: image/%s\r\n\r\n", content_type);
    }
    else if (strcmp(content_type, "jpg") == 0) {
        sprintf(buffer, "Content-Type: image/jpeg\r\n\r\n");
    }
    else {
        sprintf(buffer, "Content-Type: application/%s\r\n\r\n", content_type);
    }
}

/**
 * calc length of file
 * PARAM
 * @file file path
 * @content_len buffer where the size of file will be written
 */
void ContentLength(char* file, char* content_len)
{
    long len;
	struct stat st;

	stat(file, &st);
    len = st.st_size;

    sprintf(content_len, "Content-Length: %ld\r\n", (len + 128L));
}

/**
 * get first line of response
 * PARAM
 * @file file path
 * @buf buffer where the protocol message will be written
 */
enum Status Protocol(char* file, char* buf)
{
	sprintf(buf, "HTTP/1.1 ");
	if (access(file, F_OK) == -1) {
		strcat(buf, "404 Not Found\r\n");
		return NOT_FOUND;
	}
	else {
		strcat(buf, "200 OK\r\n");
		return OK;
	}
}
