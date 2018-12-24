all: proxy.c proxy.h proxyclient.c proxyclient.h logger.c logger.h cache.c cache.h main.c
	gcc main.c proxy.c proxyclient.c logger.c cache.c -o proxy -I./ -lpthread

debug: proxy.c proxy.h proxyclient.c proxyclient.h logger.c logger.h cache.c cache.h main.c
	gcc -g main.c proxy.c proxyclient.c logger.c cache.c -o proxy -I./ -lpthread -ggdb

clean:
	rm proxy
