#ifndef _KRPERF_SRV_H
#define _KRPERF_SRV_H
#include "krperf.h"

int server_recv(struct krperf_cb *cb, struct ib_wc *wc);

#endif /* _KRPERF_SRV_H */

