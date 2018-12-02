# Defaults
CC      ?= musl-gcc
CC_OPTS ?= -Wall -O3 -g -std=c99 -static

TARGETS=sproxy

all: $(TARGETS)

sproxy: sproxy.c config.h
	$(CC) $(CC_OPTS) -o sproxy sproxy.c

install: $(TARGETS)
	install $(TARGETS) /usr/local/bin/

clean:
	rm -f *~ $(TARGETS)
