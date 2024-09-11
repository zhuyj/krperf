#include "krperf_clt.h"

extern int debug;
#define DEBUG_LOG if (debug) printk

int krperf_client_recv(struct krperf_cb *cb, struct ib_wc *wc)
{
	if (wc->byte_len != sizeof(cb->recv_buf)) {
		pr_err("Received bogus data, size %d\n", wc->byte_len);
		return -EINVAL;
	}

	if (cb->state == RDMA_READ_ADV)
		cb->state = RDMA_WRITE_ADV;
	else
		cb->state = RDMA_WRITE_COMPLETE;

	return 0;
}

static void krperf_format_send(struct krperf_cb *cb, u64 buf)
{
	struct krperf_rdma_info *info = &cb->send_buf;
	u32 rkey;

	/*
	 * Client side will do reg or mw bind before
	 * advertising the rdma buffer. Server side
	 * sends have no data.
	 */
	if (!cb->server) {
		rkey = krperf_rdma_rkey(cb, buf, !cb->server_invalidate);
		info->buf = htonll(buf);
		info->rkey = htonl(rkey);
		info->size = htonl(cb->size);
		DEBUG_LOG("RDMA addr %llx rkey %x len %d\n",
			  (unsigned long long)buf, rkey, cb->size);
	}
}

void krperf_test_client(struct krperf_cb *cb)
{
	int ping, start, cc, i, ret;
	const struct ib_send_wr *bad_wr;
	unsigned char c;

	start = 65;
	for (ping = 0; !cb->count || ping < cb->count; ping++) {
		cb->state = RDMA_READ_ADV;

		/* Put some ascii text in the buffer. */
		cc = sprintf(cb->start_buf, "rdma-ping-%d: ", ping);
		for (i = cc, c = start; i < cb->size; i++) {
			cb->start_buf[i] = c;
			c++;
			if (c > 122)
				c = 65;
		}
		start++;
		if (start > 122)
			start = 65;
		cb->start_buf[cb->size - 1] = 0;

		krperf_format_send(cb, cb->start_dma_addr);
		if (cb->state == KRPERF_ERROR) {
			pr_err("krperf_format_send failed\n");
			break;
		}
		ret = ib_post_send(cb->qp, &cb->sq_wr, &bad_wr);
		if (ret) {
			pr_err("post send error %d(%pe)\n", ret, ERR_PTR(ret));
			break;
		}

		/* Wait for server to ACK */
		wait_event_interruptible(cb->sem, cb->state >= RDMA_WRITE_ADV);
		if (cb->state != RDMA_WRITE_ADV) {
			pr_err("wait for RDMA_WRITE_ADV state %d\n",
			       cb->state);
			break;
		}

		krperf_format_send(cb, cb->rdma_dma_addr);
		ret = ib_post_send(cb->qp, &cb->sq_wr, &bad_wr);
		if (ret) {
			pr_err("post send error %d(%pe)\n", ret, ERR_PTR(ret));
			break;
		}

		/* Wait for the server to say the RDMA Write is complete. */
		wait_event_interruptible(cb->sem, 
					 cb->state >= RDMA_WRITE_COMPLETE);
		if (cb->state != RDMA_WRITE_COMPLETE) {
			pr_err("wait for RDMA_WRITE_COMPLETE state %d\n",
			       cb->state);
			break;
		}

		if (cb->validate)
			if (memcmp(cb->start_buf, cb->rdma_buf, cb->size)) {
				pr_err("data mismatch!\n");
				break;
			}

		if (cb->verbose)
			printk(KERN_INFO PFX "ping data (64B max): |%.64s|\n",
				cb->rdma_buf);
#ifdef SLOW_KRPERF
		wait_event_interruptible_timeout(cb->sem, cb->state == KRPERF_ERROR, HZ);
#endif
	}
}

int krperf_connect_client(struct krperf_cb *cb)
{
	struct rdma_conn_param conn_param;
	int ret;

	memset(&conn_param, 0, sizeof conn_param);
	conn_param.responder_resources = 1;
	conn_param.initiator_depth = 1;
	conn_param.retry_count = 10;

	ret = rdma_connect(cb->cm_id, &conn_param);
	if (ret) {
		pr_err("rdma_connect error %d(%pe)\n", ret, ERR_PTR(ret));
		return ret;
	}

	wait_event_interruptible(cb->sem, cb->state >= KRPERF_CONNECTED);
	if (cb->state == KRPERF_ERROR) {
		pr_err("wait for CONNECTED state %d\n", cb->state);
		return -1;
	}

	DEBUG_LOG("rdma_connect successful\n");
	return 0;
}

int krperf_bind_client(struct krperf_cb *cb)
{
	struct sockaddr_storage sin;
	int ret;

	krperf_fill_sockaddr(&sin, cb);

	ret = rdma_resolve_addr(cb->cm_id, NULL, (struct sockaddr *)&sin, 2000);
	if (ret) {
		pr_err("rdma_resolve_addr error %d(%pe)\n", ret, ERR_PTR(ret));
		return ret;
	}

	wait_event_interruptible(cb->sem, cb->state >= ROUTE_RESOLVED);
	if (cb->state != ROUTE_RESOLVED) {
		pr_err("addr/route resolution did not resolve: state %d\n",
		       cb->state);
		return -EINTR;
	}

	if (!krperf_reg_supported(cb->cm_id->device))
		return -EINVAL;

	DEBUG_LOG("rdma_resolve_addr - rdma_resolve_route successful\n");
	return 0;
}
