KSRC=/lib/modules/`uname -r`/build
KOBJ=/lib/modules/`uname -r`/build

obj-m += rdma_krperf.o

CFLAGS_krperf.o = -I$(src)
CFLAGS_krperf_srv.o = -I$(src)
CFLAGS_krperf_clt.o = -I$(src)

rdma_krperf-y	:= krperf_getopt.o \
			krperf.o \
			krperf_srq.o \
			krperf_proc.o \
			krperf_srv.o \
			krperf_clt.o

default:
	make -C $(KSRC) M=`pwd` modules

install:
	make -C $(KSRC) M=`pwd` modules_install
	depmod -a

clean:
	rm -f *.o
	rm -f *.ko
	rm -f rdma_krperf.mod.c rdma_krperf.mod
	rm -f Module.symvers
	rm -f Module.markers
	rm -f modules.order
