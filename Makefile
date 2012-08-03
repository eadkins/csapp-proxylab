CC = gcc
#CFLAGS = -g -Wall -Werror -pg
#LDFLAGS = -lpthread -pg
CFLAGS = -g -Wall -Werror
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c proxylib.h
	$(CC) $(CFLAGS) -c proxy.c

proxylib.o: proxylib.c proxylib.h
	$(CC) $(CFLAGS) -c proxylib.c

cachelib.o: cachelib.c cachelib.h
	$(CC) $(CFLAGS) -c cachelib.c


hashlib.o: hashlib.c hashlib.h
	$(CC) $(CFLAGS) -c hashlib.c

proxy: proxy.o csapp.o proxylib.o cachelib.o hashlib.o

submit:
	(make clean; cd ..; tar cvf proxylab.tar proxylab-handout)

clean:
	rm -f *~ *.o proxy core

