# simple proxy

[![CircleCI](https://circleci.com/gh/FOSDEM/video-sproxy.svg?style=svg)](https://circleci.com/gh/FOSDEM/video-sproxy)

Usage: ffmpeg... | ./sproxy

It will listen on port 8899 and send whatever is currently being received from
stdin. It'll also drop slow readers.

Also, it'll listen on port 8898 and will try to send half of whatever is in
its buffer, so some tasks happen quicker (like finding a key frame to show
something).

And, it will listen on port 8080, and will respond with a HTTP reply that's
pretty much the same as 8899.

Currently the make file builds a static binary with musl, you can switch
to glibc for local debugging/testing.
