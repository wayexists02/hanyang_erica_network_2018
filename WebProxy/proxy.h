#ifndef __PROXY_H__
#define __PROXY_H__

#define BACKLOG 10

// client object
struct Client
{
	int Socket; // socket to client
	int Server; // socket to server
	char IP[32]; // buffer for storing ip address
	int Closed; // bool value for representing wheather this client is closed
	struct Client *Next; // linked list
	struct Client *Prev; // linked list
};

// function prototype
int StartServer(int port);
int OpenProxyServer(int port);
int ConnectToServer(struct Client *client, const char *host, const char *port);
struct Client *Accept();
void AddClientToList(struct Client *client);
void CheckForClosed();
void RemoveClient(struct Client *client);
void DestroyClient(struct Client *client);
void CloseProxy();

#endif
