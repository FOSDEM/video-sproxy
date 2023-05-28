# Defaults
CC_OPTS ?= -Wall -O3 -g

TARGETS=sproxy wait_next_second usb_reset

all: $(TARGETS)

wait_next_second: wait_next_second.c
	$(CC) $(CC_OPTS) -o $@ $^

usb_reset: usb_reset.c
	$(CC) $(CC_OPTS) -o $@ $^

sproxy: sproxy.c config.h
	$(CC) $(CC_OPTS) -o $@ sproxy.c

install: $(TARGETS)
	install $(TARGETS) /usr/local/bin/

clean:
	rm -f *~ $(TARGETS)
