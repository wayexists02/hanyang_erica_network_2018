#ifndef __PROXYCLIENT_H__
#define __PROXYCLIENT_H__

// function prototype
void *HandleClient(void *args);
void ParseRequest(const char *request, char *method, char *uri);
void NewRequest(const char *request, const char *method, const char *path, char *new_req);
void GetHost(const char *request, char *host, char *port);
void GetFilePath(const char *uri, const char *host, const char *port, char *path);
void CloseClient(struct Client *client);
void Logging(struct Client *client, const char *host, int size);
void CacheLog(struct Client *client, const char *host, int size);

#endif
