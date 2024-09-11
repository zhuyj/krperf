#ifndef _KRPERF_CLT_H
#define _KRPERF_CLT_H

#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>

#include "krperf.h"

int krperf_client_recv(struct krperf_cb *cb, struct ib_wc *wc);
void krperf_test_client(struct krperf_cb *cb);
int krperf_connect_client(struct krperf_cb *cb);
int krperf_bind_client(struct krperf_cb *cb);

#endif /* _KRPERF_CLT_H */
