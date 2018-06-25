#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/param.h>
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

void nonblock(int fd) {
	long flags;
	flags = fcntl( fd, F_GETFL, 0 );
	fcntl( fd, F_SETFL, flags | O_NONBLOCK );
}

struct receiver {
	int fd;
	size_t pos;
	int state;
};

int main(){
	struct receiver receivers[MAXFD];
	fd_set reads, writes, other;
	int acceptfd;
	int i;
	ssize_t sz;
	struct sockaddr_in saddr;
	struct timeval to;

	uint8_t buffer[BUFFSIZE];

	// setup stdin

	memset(receivers, 0, sizeof(receivers));
	memset(buffer, 0, sizeof(buffer));

	receivers[0].fd = STDIN_FILENO;
	receivers[0].state = STATE_READING;

	nonblock(receivers[0].fd);

	// setup accept socket

	ASSERTF_E((acceptfd = socket(AF_INET, SOCK_STREAM, 0))!=-1, "socket");

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(PORT);
	saddr.sin_family = AF_INET;

	setsockopt(acceptfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
#ifdef SO_REUSEPORT
	setsockopt(acceptfd, SOL_SOCKET, SO_REUSEPORT, &(int){ 1 }, sizeof(int));
#endif

	ASSERTF_E(bind(acceptfd, (struct sockaddr *) &saddr, sizeof(saddr))==0, "bind");
	ASSERTF_E(listen(acceptfd, 2)!=-1, "listen");

	nonblock(acceptfd);

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
		FD_SET(acceptfd, &reads);

		select (MAXFD, &reads, &writes, &other, &to);

		if (FD_ISSET(acceptfd, &reads)) {
			for (i=0;i<MAXFD;i++) if (receivers[i].state == STATE_UNUSED) break;
			ASSERTF(receivers[i].state == STATE_UNUSED, "no sockets available");
			receivers[i].state=STATE_WRITING;
			receivers[i].pos=receivers[0].pos;
			receivers[i].fd=accept(acceptfd, NULL, NULL);
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
				receivers[i].pos = (receivers[i].pos + sz) % BUFFSIZE;
				continue;
			}
		}

	}

	return 0;
}
