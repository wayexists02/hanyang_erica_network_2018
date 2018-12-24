#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include "logger.h"
#include "proxy.h"
#include "proxyclient.h"

static int ProxyServer; // descriptor for proxy server
static struct Client* Head; // linked list of clients

/**
 * start server
 * PARAM
 * @port port number
 */
int StartServer(int port)
{
	struct Client *client;
	pthread_t tid;

	// open proxy server
	if (OpenProxyServer(port) < 0) {
		CloseProxy();
		return -1;
	}

	// create logger I need
	CreateLogger("proxy.log");
	CreateLogger("cache_access.log");

	// main loop for concurrency.
	for (;;) {
		// accept client
		if ((client = Accept()) == NULL)
			continue;

		// add client to linked list
		AddClientToList(client);

		// start new thread
		pthread_create(&tid, NULL, HandleClient, client);
		pthread_detach(tid);

		// check for closed client. and remove from list
		CheckForClosed();
	}

	CloseProxy();
	return 0;
}

/**
 * open proxy server
 * PARAM
 * @port port number
 */
int OpenProxyServer(int port)
{	
	struct sockaddr_in serv_addr;

	// create socket
	if ((ProxyServer = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("ERROR: cannot open a proxy server\n");
		return -1;
	}

	// initialize address
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	// bind to address
	if (bind(ProxyServer, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR: bind error\n");
		return -1;
	}

	// listen
	if (listen(ProxyServer, BACKLOG) < 0) {
		perror("ERROR: listen error\n");
		return -1;
	}
}

/**
 * Connect to remote server
 * PARAM
 * @client client object
 * @host host name
 * @port port number to connect to remote server
 */
int ConnectToServer(struct Client *client, const char *host, const char *port)
{
	struct sockaddr_in sin;
	struct hostent* hent;

	// construct socket
	if ((client->Server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("ERROR: cannot create socket to remote server\n");
		return -1;
	}
	
	// get host by name; DNS
	if ((hent = gethostbyname(host)) == NULL) {
		fprintf(stderr, "ERROR: cannot find such a host\n");
		return -1;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	memcpy(&sin.sin_addr.s_addr, hent->h_addr, hent->h_length);
	sin.sin_port = htons(atoi(port));

	// connect to server
	if (connect(client->Server, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror("ERROR: cannot connect to server\n");
		return -1;
	}

	return 0;
}

/**
 * accept a client
 */
struct Client *Accept()
{
	struct Client *client;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_len;

	client = (struct Client *) malloc(sizeof(struct Client));
	memset(client, 0, sizeof(*client));

	clnt_len = sizeof(clnt_addr);

	// accept a client
	if ((client->Socket = accept(ProxyServer, (struct sockaddr *) &clnt_addr, &clnt_len)) < 0) {
		perror("ERROR: accept error\n");
		return NULL;
	}

	// get ip address
	sprintf(client->IP, "%s", inet_ntoa(clnt_addr.sin_addr));

	return client;
}

/**
 * add client to linked list
 */
void AddClientToList(struct Client *client)
{
	if (Head) {
		client->Next = Head;
		Head->Prev = client;
		Head = client;
	}
	else {
		Head = client;
	}
}

/**
 * checking for existing closed client.
 * if closed client detected, remove it from the list
 * this function should be executed in main thread
 */
void CheckForClosed()
{
	struct Client *cur = Head, *temp;

	while (cur) {
		temp = cur->Next;
		if (cur->Closed)
			RemoveClient(cur);

		cur = temp;
	}
}

/**
 * use to remove a client from list
 */
void RemoveClient(struct Client *client)
{
	if (Head == client && client->Next == NULL) {
		Head = NULL;
	}
	else if (Head == client) {
		client->Next->Prev = NULL;
		Head = client->Next;
	}
	else {
		if (client->Prev)
			client->Prev->Next = client->Next;
		if (client->Next)
			client->Next->Prev = client->Prev;
	}
	free(client);
}

/**
 * use to destroy all clients at exit
 * destroy client
 */
void DestroyClient(struct Client *client)
{
	if (client->Prev)
		client->Prev->Next = client->Next;
	if (client->Next)
		client->Next->Prev = client->Prev;

	if (!client->Closed) {
		close(client->Socket);
		close(client->Server);
	}
	free(client);
}

/**
 * close proxy server
 */
void CloseProxy()
{
	struct Client *cur = Head, *temp;

	// close server
	if (ProxyServer)
		close(ProxyServer);

	// destroy all clients
	while (cur) {
		temp = cur->Next;
		DestroyClient(cur);
		cur = temp;
	}
}
