#!/bin/sh
set -x

if [ "$1" = "rxe" ]; then
    NIC_NAME=`ip -o -4 a | grep -i $2 | awk '{print $2}'`
    rdma link add rxe0 type rxe netdev $NIC_NAME
    rdma link
    modprobe -v -r rdma_krperf
    sleep 2
    modprobe -v rdma_krperf
    sleep 2
    nohup echo "server,addr=$2,port=9999,verbose,count=3" > /proc/krperf &
    echo "client,addr=$2,port=9999,verbose,count=3" > /proc/krperf
    set +x
    exit 0
fi

modprobe -v -r rdma_krperf
sleep 2
modprobe -v rdma_krperf
sleep 2
nohup echo "server,addr=$1,port=9999,verbose,count=3" > /proc/krperf &
echo "client,addr=$1,port=9999,verbose,count=3" > /proc/krperf

sleep 2

modprobe -v -r rdma_krperf
sleep 2
modprobe -v rdma_krperf
sleep 2

nohup echo "server,addr=$1,port=9999,verbose,count=3,srq" > /proc/krperf &
echo "client,addr=$1,port=9999,verbose,count=3,srq" > /proc/krperf

sleep 2
modprobe -v -r rdma_krperf
sleep 2
modprobe -v rdma_krperf
sleep 2

nohup echo "server,addr=$1,port=9999,verbose,count=3" > /proc/krperf &
echo "client,addr=$1,port=9999,verbose,count=3,srq" > /proc/krperf

sleep 2
modprobe -v -r rdma_krperf
sleep 2
modprobe -v rdma_krperf
sleep 2

nohup echo "server,addr=$1,port=9999,verbose,count=3,srq" > /proc/krperf &
echo "client,addr=$1,port=9999,verbose,count=3" > /proc/krperf

set +x
