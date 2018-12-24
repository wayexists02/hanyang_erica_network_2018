#ifndef __LOGGER_H__
#define __LOGGER_H__

#define NUM_LOGGER 10
#define LEN_NAME 256

// logger object class
struct Logger
{
	char Filename[LEN_NAME]; // file name
	int Logfile; // file descriptor of log file
	struct Logger* Next; // linked list
};

// function prototype
struct Logger* CreateLogger(const char* logfilename);
void DestroyLogger(struct Logger* logger);
void DestroyAllLoggers();
void Log(struct Logger* logger, const char* msg);
struct Logger* GetLogger(const char* logfilename);
void PrintLoggers();

#endif
