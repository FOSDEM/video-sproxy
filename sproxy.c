#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/param.h>
#include <sys/select.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <err.h>

#include "config.h"

#define err_x(x...) errx(1, ## x)
#define err_e(x...) err(errno, ## x)

// Assert with errx, no errno
#define ASSERTF(c, x...) if (!(c)) errx(1, ## x)

// Assert and print errno (for syscalls)
#define ASSERTF_E(c, x...) if (!(c)) err(errno, ## x)

#ifdef DEBUG
#define debugprintf(x...) if(debug) fprintf(stderr, ## x)
#else
#define debugprintf(x...)
#endif

#define STATE_UNUSED 0
#define STATE_READING 1
#define STATE_WRITING 2

#define HTTP_REPLY "HTTP/1.0 200 OK\r\n\r\nServer: sproxy v"  VERSION  "\r\nConnection: close\r\nContent-Type: octet/stream\r\n\r\n"

void nonblock(int fd) {
	long flags;
	flags = fcntl( fd, F_GETFL, 0 );
	fcntl( fd, F_SETFL, flags | O_NONBLOCK );
}

struct receiver {
	int fd;
	size_t pos;
	int state;
	char *extra;
	size_t extralen;
};

struct accepter {
	int fd;
	int port;
	int http;
	int bufpos;
};

int getlistenfd(int port) {
	int fd;
	struct sockaddr_in6 saddr;

	ASSERTF_E((fd = socket(AF_INET6, SOCK_STREAM, 0))!=-1, "socket");

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin6_addr = in6addr_any;
	saddr.sin6_port = htons(port);
	saddr.sin6_family = AF_INET6;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
#ifdef SO_REUSEPORT
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &(int){ 1 }, sizeof(int));
#endif

	setsockopt(fd, SOL_SOCKET, IPV6_V6ONLY, &(int){ 0 }, sizeof(int));

	ASSERTF_E(bind(fd, (struct sockaddr *) &saddr, sizeof(saddr))==0, "bind");
	ASSERTF_E(listen(fd, 2)!=-1, "listen");

	nonblock(fd);

	return fd;

}

void setupaccepter(struct accepter *acc, int port, int http, int bufpos) {
	acc->port = port;
	acc->http = http;
	acc->bufpos = bufpos;
	acc->fd = getlistenfd(port);
}

#define ACCEPTERS 3

int main(int argc, char **argv){
	struct receiver receivers[MAXFD+1];
	struct accepter accepters[MAXFD];
	fd_set reads, writes, other;
	int i;
	ssize_t sz;
	struct timeval to;

	uint8_t buffer[BUFFSIZE];

	if (argc>1 && !strcmp(*(argv+1), "--version")) {
		printf("%s\n", VERSION);
		exit(0);
	}

	memset(receivers, 0, sizeof(receivers));
	memset(buffer, 0, sizeof(buffer));
	memset(accepters, 0, sizeof(accepters));

	// setup stdin
	receivers[0].fd = STDIN_FILENO;
	receivers[0].state = STATE_READING;

	nonblock(receivers[0].fd);

	setupaccepter(&accepters[0], PORT, 0, 0);
	setupaccepter(&accepters[1], PORTBUF, 0, 1);
	setupaccepter(&accepters[2], PORTHTTP, 1, 0);

	signal(SIGPIPE, SIG_IGN);

	while(42) {
		FD_ZERO(&reads);
		FD_ZERO(&writes);
		FD_ZERO(&other);
		
		to.tv_sec=0;
		to.tv_usec=100*1000;

		for (i=0; i<MAXFD; i++) {
			if (receivers[i].state == STATE_UNUSED) continue;
			if (receivers[i].state == STATE_READING) FD_SET(receivers[i].fd, &reads);
			if (receivers[i].state == STATE_WRITING && receivers[i].pos!=receivers[0].pos) FD_SET(receivers[i].fd, &writes);
		}

		for (int l=0;accepters[l].fd!=0;l++) FD_SET(accepters[l].fd, &reads);

		select (MAXFD, &reads, &writes, &other, &to);

		for (int l=0;accepters[l].fd!=0;l++) {
			if (FD_ISSET(accepters[l].fd, &reads)) {
				for (i=0;i<MAXFD;i++) 
					if (receivers[i].state == STATE_UNUSED) break;

				if (receivers[i].state != STATE_UNUSED) {
					int rfd = accept(accepters[l].fd, NULL, NULL);
					close(rfd);
				} else {
					receivers[i].state=STATE_WRITING;
					receivers[i].fd=accept(accepters[l].fd, NULL, NULL);
					nonblock(receivers[i].fd);

					if (accepters[l].bufpos == 0) {
						receivers[i].pos=receivers[0].pos;
					} else {
						// start not from the current posistion, but somewhere in the back
						receivers[i].pos=(receivers[0].pos+BUFFSIZE/2) % BUFFSIZE;
					}

					if (accepters[l].http !=0) {
						receivers[i].extra = HTTP_REPLY;
						receivers[i].extralen = strlen(HTTP_REPLY);
					}
				}
			}
		}

		for (i=0; i<MAXFD; i++) {
			// hateswitchcase
			if (receivers[i].state == STATE_UNUSED) continue;
			if (receivers[i].state == STATE_READING && FD_ISSET(receivers[i].fd, &reads)) {
				sz = read(receivers[i].fd, &buffer[receivers[i].pos], BUFFSIZE-receivers[i].pos);
				ASSERTF(sz!=0, "eof on stdin");
				if (sz==-1 && errno!=EAGAIN) {
					ASSERTF_E(1, "read");
				}
				if (sz == -1 && errno==EAGAIN) {
					continue;
				}
				for (int j=0; j<MAXFD; j++) {
					if (receivers[j].state == STATE_WRITING && receivers[j].pos>receivers[i].pos && receivers[j].pos<=receivers[i].pos+sz ) {
						close(receivers[j].fd);
						debugprintf("client %d fd %d dropped\n", j, receivers[j].fd);
						memset(&receivers[j], 0, sizeof(struct receiver));
					}
				}
				receivers[i].pos = (receivers[i].pos + sz) % BUFFSIZE;
				continue;
			}

			if (receivers[i].state == STATE_WRITING && FD_ISSET(receivers[i].fd, &writes)) {
				if (receivers[i].extra!=NULL && receivers[i].extralen!=0) {
					sz = write(receivers[i].fd, receivers[i].extra, receivers[i].extralen);
					if (sz == -1 && errno!=EAGAIN) {
						close(receivers[i].fd);
						debugprintf("client %d fd %d died\n", i, receivers[i].fd);
						memset(&receivers[i], 0, sizeof(struct receiver));
					}
					if (sz > 0) {
						receivers[i].extralen -= sz;
						receivers[i].extra += sz;
					}
					if (receivers[i].extralen<=0) {
						receivers[i].extra = NULL;
						receivers[i].extralen = 0;
					}
					continue;
				
				}
				ssize_t writesize = receivers[0].pos-receivers[i].pos;
				if (writesize<0) writesize = BUFFSIZE-receivers[i].pos;
				if (writesize==0) continue;
				sz = write(receivers[i].fd, &buffer[receivers[i].pos], writesize);
				//debugprintf("cli %d fd %d start %ld end %ld actual %ld\n", i, receivers[i].fd, receivers[i].pos, MIN(BUFFSIZE-receivers[i].pos, receivers[0].pos), sz);
				if (sz == -1 && errno!=EAGAIN) {
					close(receivers[i].fd);
					debugprintf("client %d fd %d died\n", i, receivers[i].fd);
					memset(&receivers[i], 0, sizeof(struct receiver));
				}
				if (sz > 0) {
					receivers[i].pos = (receivers[i].pos + sz) % BUFFSIZE;
				}
				continue;
			}
		}

	}

	return 0;
}
