#!/bin/bash
# join.sh - demonstration script to send a desired IGMP membership report.

if [ "$#" -ne 2 ]; then echo "Usage: join.sh interface mcast_addr"; exit 1; fi
INTERFACE="$1"
MCAST_ADDR="$2"

# Parse out each octet from the IP addr
pattern='^([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)$'
if [[ "$MCAST_ADDR" =~ $pattern ]]; then
  HEX=$(printf '%02x%02x%02x%02x' ${BASH_REMATCH[1]} ${BASH_REMATCH[2]} ${BASH_REMATCH[3]} ${BASH_REMATCH[4]})
else :
  echo "invalid IP addr"; exit 1
fi

PKT="01005e000016111111111111080046c00028000040000102000011111111e000001694040000220011110000000104000000$HEX"
./raw_send -c -e -i -g $INTERFACE $PKT
