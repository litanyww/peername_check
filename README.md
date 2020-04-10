# Tester for `getpeername()`

This project opens a connection to a remote hostname on port 80, and attempts
to retreive a non-existant URL.  It will report the IP address it has connected
to by calling the `getpeername()` function.

Build the project by running `make`.  This will create two otherwise identical
executables; 'Safari' and 'notSafari'.

## Why do this?

The Sophos Web Intelligence product will intercept outgoing network connections
from a process if it has a name which it recognizes as a browser.  This
connection is redirected to a local port.  However, the kext which performs
this interception will also intercept calls to `getpeername()`, and should be
able to fool the browser into thinking that its connection has not been
intercepted.

## Is it working?

You can confirm whether the connection has been intercepted through the use of
tcpdump:

    sudo tcpdump -i all -s 0 -w /tmp/dump.pcap
    tcpdump -r /tmp/dump.pcap -v -A

An intercepted connection will appear in the dump as a connection on the local
interface, and a second, nominally identical stream on the external interface.
The notSafari binary should not be intercepted, while the 'Safari' binary will
be if Sophos Web Intelligence is active.
