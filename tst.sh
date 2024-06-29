#!/bin/sh
# tst.sh

tcpdump -i em1 -w x.pcap --immediate-mode &
TCPDUMP_PID=$!

sleep 0.2

# IGMP membership report packet. Send as-is.
/usr/local/bin/raw_send             em1 01005e000016000f53797050080046c00028000040000102f5770a1d0465e0000016940400002200e78d0000000104000000ef65030b

# IGMP membership report packet. Update source MAC and IP address and checksums.
/usr/local/bin/raw_send -c -e -i -g em1 01005e000016111111111111080046c00028000040000102000011111111e000001694040000220011110000000104000000ef65030b

# UDP multicast packet sent to port 12000 with payload "123".
/usr/local/bin/raw_send             em1 01005e65030be454e884f33b08004500001fd43440000f1197a70a1d0365ef65030bddbd2ee0000b8f15313233

# Same UDP packet. Update source MAC and IP address and checksums.
/usr/local/bin/raw_send -c -e -i -u em1 01005e65030b11111111111108004500001fd43440000f11111111111111ef65030bddbd2ee0000b1111313233

sleep 0.2

kill $TCPDUMP_PID
