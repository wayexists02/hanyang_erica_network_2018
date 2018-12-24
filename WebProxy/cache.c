#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "logger.h"

// store object as linked list
static struct Object* Head = NULL;
static struct Object* Tail = NULL;
static int Size = 0; // size currently stored

/**
 * get object from cache
 */
struct Object* GetFromCache(const char *uri)
{
	struct Object *cur = Head;

	while (cur) {
		if (strcmp(cur->Uri, uri) == 0) {

			// referenced object must be gone to Head (LRU)
			if (Head == cur && Tail == cur) {
				// forward
			}
			else if (Head == cur) {
				// forward
			}
			else if (Tail == cur) {
				Tail = cur->Prev;
				Tail->Next = NULL;
				cur->Prev = NULL;

				cur->Next = Head;
				Head->Prev = cur;
				Head = cur;
			}
			else {
				cur->Next->Prev = cur->Prev;
				cur->Prev->Next = cur->Next;

				cur->Prev = NULL;
				cur->Next = Head;
				Head->Prev = cur;
				Head = cur;
			}

			// return content
			return cur;
		}
		cur = cur->Next;
	}

	return NULL;
}

/**
 * add object to cache
 */
void AddToCache(const char* uri, const void* content, int sz)
{
	struct Object *newobj,  *temp;

	// create new object
	newobj = (struct Object*) malloc(sizeof(struct Object));
	newobj->Uri = (char*) malloc(sizeof(char) * (strlen(uri) + 1));
	newobj->Content = malloc(sizeof(char) * sz);
	newobj->Prev = NULL;
	newobj->Next = NULL;

	// set uri and size of object
	memset(newobj->Uri, 0, sizeof(char) * (strlen(uri) + 1));
	strcpy(newobj->Uri, uri);
	memcpy(newobj->Content, content, sz);
	newobj->Size = sz;

	// replacement
	while (Size + sz > MAX_CACHE_SIZE) {
		temp = Head->Next;
		temp->Prev = NULL;
		Head->Next = NULL;

		Size -= Head->Size;
		
		free(Head->Content);
		free(Head->Uri);
		free(Head);
		Head = temp;
	}

	// add to new object to head
	if (Head == NULL) {
		Head = Tail = newobj;
	}
	else {
		Tail->Next = newobj;
		newobj->Prev = Tail;
		Tail = newobj;
	}
	Size += newobj->Size;
}

// free all object stored in cache
void FreeAllCache()
{
	struct Object *cur = Head;
	
	while (cur) {
		if (cur->Prev)
			free(cur->Prev);
		free(cur->Uri);
		free(cur->Content);
		cur = cur->Next;
	}
	Size = 0;
	Head = Tail = NULL;
}

// DEBUG print all object
void PrintAll()
{
	struct Object *cur = Head;
	char str[1024];
	int i;

	memset(str, 0, 1024);
	CreateLogger("CacheInfo.log");

	i = 1;
	while (cur) {
		sprintf(str, "CACHE: %d, %s", i++, cur->Uri);
		Log(GetLogger("CacheInfo.log"), str);
		cur = cur->Next;
	}
}
