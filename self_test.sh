#!/bin/sh
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

