# raw_send

Program to send any kind of Ethernet packet.


# Table of contents

<!-- mdtoc-start -->
&bull; [raw_send](#raw_send)  
&bull; [Table of contents](#table-of-contents)  
&nbsp;&nbsp;&nbsp;&nbsp;&bull; [License](#license)  
&nbsp;&nbsp;&nbsp;&nbsp;&bull; [Introduction](#introduction)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&bull; [Help](#help)  
&nbsp;&nbsp;&nbsp;&nbsp;&bull; [Wireshark 'Copy as Hex Stream'](#wireshark-copy-as-hex-stream)  
&nbsp;&nbsp;&nbsp;&nbsp;&bull; [Update Sender Addresses](#update-sender-addresses)  
&nbsp;&nbsp;&nbsp;&nbsp;&bull; [Update UDP Data](#update-udp-data)  
&nbsp;&nbsp;&nbsp;&nbsp;&bull; [Update IGMP Data](#update-igmp-data)  
<!-- TOC created by '/home/sford/bin/mdtoc.pl README.md' (see https://github.com/fordsfords/mdtoc) -->
<!-- mdtoc-end -->

## License

I want there to be NO barriers to using this code, so I am releasing it to the public domain.  But "public domain" does not have an internationally agreed upon definition, so I use CC0:

Copyright 2024 Steven Ford http://geeky-boy.com and licensed
"public domain" style under
[CC0](http://creativecommons.org/publicdomain/zero/1.0/):
![CC0](https://licensebuttons.net/p/zero/1.0/88x31.png "CC0")

To the extent possible under law, the contributors to this project have
waived all copyright and related or neighboring rights to this work.
In other words, you can use this code for any purpose without any
restrictions.  This work is published from: United States.  The project home
is https://github.com/fordsfords/raw_send

To contact me, Steve Ford, project owner, you can find my email address
at http://geeky-boy.com.  Can't see it?  Keep looking.


## Introduction

The "raw_send" program opens a raw socket and sends a packet that you specify
on the command line. The packet is specified as a hexidecimal string and must
include Ethernet headers, IP headers, etc.

This program must run with root privilages.


### Help

raw_send -h
Usage: raw_send [-h] [-e] [-c] [-g] [-i] [-u] interface hex_data
Where:
-h : help
-c : overwrite IP checksum with calculated value.
-e : overwrite ethernet source address with interface MAC.
-g : overwrite IGMP checksum with calculated value.
-i : overwrite IP source address with interface address.
-u : overwrite UDP checksum with calculated value.
interface : device name (e.g. 'eth0')
hex_data : hex string with no spaces containing Ethernet headers, IP, etc.


## Wireshark 'Copy as Hex Stream'

A common way of using this tool is to capture packets and use WireShark to
display a packet you want to reproduce.
If you select a packet, right click on the "Frame" section of the dissector,
and select "Copy" -> "...As a Hex Stream", it will copy the hexidecimal
of the packet into your cut buffer.
You can ten paste it as the "hex_data" parameter to raw_send.
For example, here's a "raw_send" command with an "igmp report" packet
pasted as the hex data:
````
raw_send em1 01005e000016000f53797050080046c00028000040000102f5770a1d0465e0000016940400002200e78d0000000104000000ef65030b
````
The hex_data parameter was taken from a WireShark capture of an "igmp report".
It includes the Ethernet header (including source and destination MAC
addresses), IP header, IP checksum, etc.
The raw_send tool will send the packet as-is.

Where "em1" is the interface name.

Note that this program requires root privilages to work.

## Update Sender Addresses

If you have a packet capture taken elsewhere and you want to send the
same packets from your machine, you probably want to update the Ethernet
MAC address and IP address of the source.
This is painful to do by hand, but the "-e -i -c" flags make it easy.
It overwrites the Ethernet header's source MAC address with the MAC
address of the desired interface, ditto with the source IP address,
and recalculates the IP checksum.

## Update UDP Data

Hand-editing the UDP data on a datagram is painful but possible.
One thing that is especially hard is re-calculating the UDP checksum.
The "-u" flag does that for you automatically.

## Update IGMP Data

Hand-editing the IGMP data on a datagram is painful but possible.
One thing that is especially hard is re-calculating the IGMP checksum.
The "-g" flag does that for you automatically.
