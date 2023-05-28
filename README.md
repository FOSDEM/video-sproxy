# simple proxy

[![CircleCI](https://circleci.com/gh/FOSDEM/video-sproxy.svg?style=svg)](https://circleci.com/gh/FOSDEM/video-sproxy)

Usage: ffmpeg... | ./sproxy [portshift]

It will listen on port 8899 and send whatever is currently being received from
stdin. It'll also drop slow readers.

Also, it'll listen on port 8898 and will try to send half of whatever is in
its buffer, so some tasks happen quicker (like finding a key frame to show
something).

And, it will listen on port 80, and will respond with a HTTP reply that's
pretty much the same as 8899.

All ports can be shifted with between 2 and 10000, if a command line option
is passed.

It's currently built dynamically (was static with musl), but we've moved to
normal plackages.

# Extras

There are two extra tools in the repository, needed by the FOSDEM video team:

* `usb_reset` - reset an USB device by the /dev/bus/usb/BUS/DEV path
* `wait_next_second` - sleep until the next exact second

# Availability

Currently, the pre-built debian packages are available at https://packagecloud.io/fosdem/video-team

# TODO

* any kind of stats reporting, maybe tracking of retransmits
* move the ports to args

# Will-not-do

* proper http implementation - there is pretty much no point, as http is an order
  of magnitude more code.
* config file - again, order of magnitude more code
* push for inclusion in Debian - on its own this is too small to waste the project's
  resources. If it gets adopted in a collection of similar tools, then it might be
  reasonable (and will get at least proper argument parsing support).
