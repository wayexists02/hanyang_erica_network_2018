#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "logger.h"
#include "proxy.h"
#include "proxyclient.h"

void *HandleClient(void *args)
{
	struct Client *client;
	char request[2048];
	char new_request[2048];
	char method[8];
	char uri[1024];
	char host[256];
	char path[1024];
	char port[8];
	char buf[256];
	struct Object *content_cache;
	void *content;
	int n, size;

	// initialize variables
	client = (struct Client *) args;
	memset(request, 0, 2048);
	memset(new_request, 0, 2048);
	memset(method, 0, 8);
	memset(uri, 0, 1024);
	memset(host, 0, 256);
	memset(path, 0, 1024);
	memset(port, 0, 8);
	memset(buf, 0, 256);

	// read request from client
	if (read(client->Socket, request, 2048) < 0) {
		perror("ERROR: cannot read request");
		CloseClient(client);
		return NULL;
	}

//	printf("%s\n", request);

	// parse request
	ParseRequest(request, method, uri);

//	printf("METHOD: %s\n", method);
//	printf("URI: %s\n", uri);

	// get host from request
	GetHost(request, host, port);
	if (strlen(host) == 0) {
		CloseClient(client);
		return NULL;
	}
	GetFilePath(uri, host, port, path);

//	printf("HOST: %s\n", host);
//	printf("PORT: %s\n", port);
//	printf("PATH: %s\n", path);

	// process only GET message
	if (strcmp(method, "GET") != 0) {
		CloseClient(client);
		return NULL;
	}

	// if the object was cached, return it
	if (content_cache = GetFromCache(uri)) {
		write(client->Socket, content_cache->Content, content_cache->Size);
		CacheLog(client, host, content_cache->Size);
	}
	else { // connect to remote server
		// create new request to send server
		NewRequest(request, method, path, new_request);
//		printf("%s\n", new_request);

		// connect to remote server
		if (ConnectToServer(client, host, port) < 0) {
			CloseClient(client);
			return NULL;
		}

		size = 0;
		content = malloc(MAX_OBJECT_SIZE + 256);

		// send new request to server
		write(client->Server, new_request, strlen(new_request));

		// forward objects
		while (n = read(client->Server, buf, 256)) {
			write(client->Socket, buf, n);

			size += n;
			if (size > MAX_OBJECT_SIZE)
				continue;

			memcpy(content + size - n, buf, n);
		}

		// if size is approvable, cache it
		if (size <= MAX_OBJECT_SIZE) {
			AddToCache(uri, content, size);
			Logging(client, host, size);
		}

		free(content);
	}

	// close client
	CloseClient(client);
	return NULL;
}

/**
 * parse request
 * PARAM
 * @request request from client
 * @method buffer for storing method
 * @uri buffer for storing uri
 */
void ParseRequest(const char *request, char *method, char *uri)
{
	char buf_method[8], buf_uri[1024];

	sscanf(request, "%s %s", buf_method, buf_uri);
	sprintf(method, "%s", buf_method);
	sprintf(uri, "%s", buf_uri);
}

/**
 * construct new request
 * PARAM
 * @request request from client
 * @method method
 * @filepath file path
 * @new_req buffer for storing new request
 */
void NewRequest(const char *request, const char *method, const char *filepath, char *new_req)
{
	char buf_req[2048];
	char *ptr;

	// make a copy of request
	sprintf(buf_req, "%s", request);

	// first line
	ptr = strtok(buf_req, "\r\n");

	sprintf(new_req, "%s %s HTTP/1.0\r\n", method, filepath);

	while (ptr = strtok(NULL, "\r\n")) {
		if (strstr(ptr, "Proxy-Connection:") != NULL) {
			continue;
		}
		else if (strstr(ptr, "Connection:") != NULL) {
			continue;
		}
		else {
			strcat(new_req, ptr);
			strcat(new_req, "\r\n");
		}
	}
	strcat(new_req, "Proxy-Connection: close\r\n");
	strcat(new_req, "Connection: close\r\n");
	strcat(new_req, "\r\n");
}

/**
 * extract host name from request
 * PARAM
 * @request request from client
 * @host buffer for storing host
 * @port buffer for storing port
 */
void GetHost(const char *request, char *host, char *port)
{
	char *ptr, *ptr_port;
	char buf[256];

	// find "Host: "
	if ((ptr = strstr(request, "Host: ")) == NULL)
		return;
	ptr += strlen("Host: ");
	sscanf(ptr, "%s", buf);

	// find host name
	ptr = strtok(buf, ":");
	sprintf(host, "%s", ptr);

	// find port
	ptr_port = strtok(NULL, ":");
	if (ptr_port == NULL)
		sprintf(port, "80");
	else
		sprintf(port, "%s", ptr_port);
}

/**
 * extract file path from uri
 * PARAM
 * @uri uri
 * @host host name
 * @port port number
 * @filepath buffer for storing file path
 */
void GetFilePath(const char *uri, const char *host, const char *port, char *filepath)
{
	char *ptr;
	char buf_port[16];

	ptr = strstr(uri, host) + strlen(host);

	sprintf(buf_port, ":%s", port);
	if (strstr(ptr, buf_port) != NULL)
		ptr += strlen(buf_port);
	sprintf(filepath, "%s", ptr);
}

/**
 * close client
 */
void CloseClient(struct Client *client)
{
	if (client->Socket)
		close(client->Socket);
	if (client->Server)
		close(client->Server);
	client->Closed = 1;
}

/**
 * log to 'proxy.log' file
 * log object which is about to cache
 */
void Logging(struct Client *client, const char *host, int size)
{
	char buf[1024];

	sprintf(buf, "%s %s %d", client->IP, host, size);
	Log(GetLogger("proxy.log"), buf);
}

/**
 * log to 'cache_access.log'
 * log record of access to object which was cached
 */
void CacheLog(struct Client *client, const char *host, int size)
{
	char buf[1024];

	sprintf(buf, "%s %s %d", client->IP, host, size);
	Log(GetLogger("cache_access.log"), buf);
}
