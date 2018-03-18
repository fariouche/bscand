# bscand
Simple sane button scanner
--------------------------

**scanbd** is mainly a fork of the saned frontend. I needed the latest backends and scanbd didn't include it yet. Plus the fact that the configuration was not easy.

I've seen too late an other project, **insaned**.  This one does not support multiple concurrent access to the scanner and does not use the net backend. Concurrent access is not easy when not using the net backend.


This project is meant to be simpler than the other projects:

1. a patch to saned, to enable concurrent access. I hope it will be accepted by the sane developers.
2. a daemon, using the net backend and the standard sane APIs.
3. All **sane** tools (xsane, scanimage etc) are still working while **bscand** is running in background.

All the **sane backends** are supported. No complex modifications needed.


Build:
------

* Apply the two patches to sane backend source code (sane backends 1.0.25 or 1.0.27)
* You need:
  * libcups2-dev
  * libconfig-dev
  * libsane-dev
* execute ``autoreconf -vfi``
* ./configure --prefix=/usr
* make
* make install


Then just modify the config file to suite your need (possible actions: Append to tiff, create pdf, print/copy using the default cups printer, scan to folder)

Then launch the daemon:

bscand -c /etc/bscand.cfg &

It will use the net backend and select the first ipv6 localhost net backend (if --enable-ipv6 is passed to configure) and if not found, search for an ipv4 localhost saned.

Note:

You will need to enable the net backend.
- compile sane-backend with the net backend support
- enable net backend, by creating /etc/sane.d/net.conf and putting inside 127.0.0.1 and ::1
- test using scanimage -L to see if the net backend is visible.



TODO:
-----
* daemonize option.
* select a different backend if multiple are found.


