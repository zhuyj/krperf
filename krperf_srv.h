#ifndef _KRPERF_SRV_H
#define _KRPERF_SRV_H
#include "krperf.h"

int krperf_server_recv(struct krperf_cb *cb, struct ib_wc *wc);
void krperf_test_server(struct krperf_cb *cb);
int krperf_bind_server(struct krperf_cb *cb);

#endif /* _KRPERF_SRV_H */
