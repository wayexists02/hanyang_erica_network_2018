#ifndef __CACHE_H__
#define __CACHE_H__

#define MAX_OBJECT_SIZE 512000
#define MAX_CACHE_SIZE 5000000

// object for storing into cache
struct Object 
{
	char* Uri; // identifier of cache object
	void* Content; // content of cache object
	int Size; // size of object
	struct Object* Next; // linked list
	struct Object* Prev; // linked list
};

// function prototype
struct Object* GetFromCache(const char* uri);
void AddToCache(const char* uri, const void* content, int sz);
void FreeAllCache();
void PrintAll();

#endif
