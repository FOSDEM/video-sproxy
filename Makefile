
# Glibc
#CC=gcc -Wall -O3 -g -std=c99

# musl-static
CC=musl-gcc -Wall -O3 -g -std=c99 -static


TARGETS=sproxy

all: $(TARGETS)

sproxy: sproxy.c config.h
	$(CC) -o sproxy sproxy.c

install: $(TARGETS)
	install $(TARGETS) /usr/local/bin/


clean:

	rm -f *~ $(TARGETS)




