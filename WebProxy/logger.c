#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "logger.h"

#define BSIZE 1024

// store loggers as linked list
static struct Logger* Head = NULL;
static struct Logger* Tail = NULL;

// create new logger object with filename
struct Logger* CreateLogger(const char* logfilename)
{
	struct Logger* logger;

	// create logger
	logger = (struct Logger*) malloc(sizeof(struct Logger));
	logger->Next = NULL;
	sprintf(logger->Filename, "%s", logfilename);

	// link logger to log file
	logger->Logfile = open(logfilename, O_WRONLY | O_CREAT);
	if (logger->Logfile < 0) {
		fprintf(stderr, "CANNOT CREATE LOG FILE!\n");
		return NULL;
	}

	// to append not overwrite
	lseek(logger->Logfile, 0, SEEK_END);

	if (Head == NULL) {
		Head = logger;
		Tail = logger;
	}
	else {
		Tail->Next = logger;
		Tail = logger;
	}

	return logger;
}

// destroy logger object
void DestroyLogger(struct Logger* logger)
{
	close(logger->Logfile);
	free(logger);
	logger = NULL;
}

// destroy all loggers
void DestroyAllLoggers()
{
	struct Logger *cur = Head, *temp;

	while (cur) {
		temp = cur->Next;
		DestroyLogger(cur);
		cur = temp;
	}

	Head = Tail = NULL;
}

// log the message
void Log(struct Logger* logger, const char* msg)
{
	char buffer[BSIZE];
	char timestr[BSIZE];
	time_t cur_time;
	struct tm* t;

	memset(buffer, 0, BSIZE);
	memset(timestr, 0, BSIZE);

	time(&cur_time);
	t = localtime(&cur_time);
	strftime(timestr, BSIZE, "%a, %b %d %Y, %X %Z", t);

	sprintf(buffer, "%s: %s\n", timestr, msg);

	write(logger->Logfile, buffer, strlen(buffer));
}

// return logger object with name
struct Logger* GetLogger(const char* logfilename)
{
	struct Logger* cur = Head;

	while (cur) {
		if (strcmp(cur->Filename, logfilename) == 0)
			return cur;
		cur = cur->Next;
	}

	return NULL;
}

void PrintLoggers()
{
	struct Logger* cur = Head;
	int i;

	i = 1;
	while (cur) {
		printf("LOGGER: %d, %s\n", i++, cur->Filename);
		cur = cur->Next;
	}
}
