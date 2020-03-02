cc=gcc
PROGS=client server

all:	$(PROGS)

client : client.c tfv2.h
	cc client.c -o client
server : server.c tfv2.h
	cc server.c -o server
clean :
	rm client server dest
