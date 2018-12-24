#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "proxy.h"
#include "cache.h"
#include "logger.h"

// function forward declaration
void CleanExit(int signo);

// main
int main(int argc, char *argv[])
{
	int errcode;
	struct sigaction intact;

	// error checking
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s [port#]\n", argv[0]);
		CleanExit(0);
	}

	// construct signal handler for clean exit
	memset(&intact, 0, sizeof(intact));
	intact.sa_handler = CleanExit;
	sigfillset(&intact.sa_mask);
	sigaction(SIGINT, &intact, NULL);
	signal(SIGPIPE, SIG_IGN);

	// print instruction
	printf("PROXY: Please press Ctrl+C to quit proxy server.\n");

	// error checking
	// I did not implement error code.
	if ((errcode = StartServer(atoi(argv[1]))) < 0) {
		fprintf(stderr, "PROXY: error code: %d\n", errcode);
		CleanExit(0);
	}

	CleanExit(0);
	return 0;
}

// for clean exit
void CleanExit(int signo)
{
	DestroyAllLoggers();
	FreeAllCache();
	CloseProxy();
	exit(0);
}
