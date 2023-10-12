#!/bin/bash
IDX=88
PORT="66$IDX"
WG_NAME="wg$IDX"
HOST_ADDR="192.168.2.1/24"
HOST_NET_PREFIX="192.168.2.0/24"
ENDPOINT="15.168.38.240:$PORT"

GUEST_ADDR="192.168.$IDX.1/24"
GUEST_NET_PREFIX="192.168.$IDX.0/24"

HOST_PUB_KEY=`cat publickey`

PRI_KEY_F="$IDX.privatekey"
PUB_KEY_F="$IDX.pubkey"
umask 077
wg genkey > $PRI_KEY_F
wg pubkey < $PRI_KEY_F > $PUB_KEY_F
GUEST_PRI_KEY=`cat $PRI_KEY_F`
GUEST_PUB_KEY=`cat $PUB_KEY_F`
rm $PUB_KEY_F $PRI_KEY_F

echo "guest PRI:$GUEST_PRI_KEY"
echo "guest PUB:$GUEST_PUB_KEY"
cat publickey
cat privatekey

ip l d $WG_NAME
ip link add $WG_NAME type wireguard
echo "wait 1 second for ip link cmd"
sleep 1
ip a a dev $WG_NAME $HOST_ADDR
wg set wg$IDX listen-port $PORT private-key privatekey peer $GUEST_PUB_KEY allowed-ips $GUEST_NET_PREFIX
echo "wg set wg$IDX listen-port $PORT private-key privatekey peer $GUEST_PUB_KEY allowed-ips $GUEST_NET_PREFIX"
ip l set $WG_NAME up
ip r a $GUEST_NET_PREFIX dev $WG_NAME


echo "-----------------------------------------------------------------"
echo ""
echo "ip l d $WG_NAME"
echo "ip link add $WG_NAME type wireguard"
echo "ip a a dev $WG_NAME $GUEST_ADDR"
echo ""
echo "rm privatekey"
echo "echo $GUEST_PRI_KEY > privatekey"
echo "wg set wg$IDX listen-port $PORT private-key privatekey peer $HOST_PUB_KEY allowed-ips $HOST_NET_PREFIX endpoint $ENDPOINT"
echo ""
echo "ip l set $WG_NAME up"
echo "ip r a $HOST_NET_PREFIX dev $WG_NAME"
