#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>

#include "krperf_srv.h"

extern int debug;
#define DEBUG_LOG if (debug) printk

int server_recv(struct krperf_cb *cb, struct ib_wc *wc)
{
	if (wc->byte_len != sizeof(cb->recv_buf)) {
		pr_err("Received bogus data, size %d\n", wc->byte_len);
		return -EINVAL;
	}

	cb->remote_rkey = ntohl(cb->recv_buf.rkey);
	cb->remote_addr = ntohll(cb->recv_buf.buf);
	cb->remote_len  = ntohl(cb->recv_buf.size);
	DEBUG_LOG("Received rkey %x addr %llx len %d from peer\n",
		  cb->remote_rkey, (unsigned long long)cb->remote_addr,
		  cb->remote_len);

	if (cb->state <= KRPERF_CONNECTED || cb->state == RDMA_WRITE_COMPLETE)
		cb->state = RDMA_READ_ADV;
	else
		cb->state = RDMA_WRITE_ADV;

	return 0;
}
