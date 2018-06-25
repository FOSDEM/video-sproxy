# simple proxy

Usage: ffmpeg... | ./sproxy

It will listen on port 8899 and send whatever is currently being received from
stdin. It'll also drop slow readers.
