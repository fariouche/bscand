# bscand
Simple sane button scanner
--------------------------

**scanbd** is mainly a fork of the saned frontend. I needed the latest backends and scnabd didn't include it yet. Plus the fact that the configuration was not easy.

I've seen too late an other project, **insaned**.  This one does not support multiple conccurrent access to the scanner and does not use the net backend. Concurrent access is not easy when not using the net backend.


This project is meant to be simpler than the other projects:

1. a patch to saned, to enable concurrent access. I hope it will be accepted by the sane developers.
2. a daemon, using the net backend and the standard sane APIs.

All the sane backends are supported. No complex modifications needed.


Build:
------

* Apply the two patches to sane backend source code (sane backends 1.0.25)
* execute ``autoreconf -vfi``
* ./configure --prefix=/usr
* make
* make install


Then just modify the config file to suite your need (possible actions: Append to tiif, create pdf, print/copy, scan to folder)

Then launch the daemon:

bscand -c /etc/bscand.cfg &

It will use the net backend and select the first ipv6 net backend (for now).


TODO:
-----
* daemonize option.
* if no ipv6, use local host ipv4 address.
* select a different backend is multiple are found.


