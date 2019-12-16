# simple proxy

[![CircleCI](https://circleci.com/gh/FOSDEM/video-sproxy.svg?style=svg)](https://circleci.com/gh/FOSDEM/video-sproxy)

Usage: ffmpeg... | ./sproxy

It will listen on port 8899 and send whatever is currently being received from
stdin. It'll also drop slow readers.

Also, it'll listen on port 8898 and will try to send half of whatever is in
its buffer, so some tasks happen quicker (like finding a key frame to show
something).

And, it will listen on port 80, and will respond with a HTTP reply that's
pretty much the same as 8899.

Currently the make file builds a static binary with musl, you can switch
to glibc for local debugging/testing.


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
