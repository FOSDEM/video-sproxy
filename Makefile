CC=gcc -Wall -O3 -g -lpthread -std=c99


TARGETS=sproxy

all: $(TARGETS)

sproxy: sproxy.c config.h
	$(CC) -o sproxy sproxy.c

install: $(TARGETS)
	install $(TARGETS) /usr/local/bin/


clean:

	rm -f *~ $(TARGETS)




