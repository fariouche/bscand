# bscand
sane button scanner

scanbd is more flexible, but it is mainly a fork of the saned frontend plus the complexity of the backends configuration.
It means that if sane adds a new backend, scanbd will need to be updated...

This project is meant to be simpler than the other projects:
1- a patch to saned, to enable concurrent access
2- a daemon, using the net backend and the standard sane APIs.

All the sane backends are supported. No complex modifications needed.

For now, bscand is hardcoded for the actions.
Later it will use a simple configuration file for custom actions.


