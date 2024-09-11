#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>

#include "krperf_srv.h"

extern int debug;
#define DEBUG_LOG if (debug) printk

int krperf_server_recv(struct krperf_cb *cb, struct ib_wc *wc)
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

void krperf_test_server(struct krperf_cb *cb)
{
	struct ib_send_wr inv;
	const struct ib_send_wr *bad_wr;
	int ret;

	while (1) {
		/* Wait for client's Start STAG/TO/Len */
		wait_event_interruptible(cb->sem, cb->state >= RDMA_READ_ADV);
		if (cb->state != RDMA_READ_ADV) {
			printk(KERN_WARNING PFX "wait for RDMA_READ_ADV state %d\n",
					cb->state);
			break;
		}

		DEBUG_LOG("server received sink adv\n");

		cb->rdma_sq_wr.rkey = cb->remote_rkey;
		cb->rdma_sq_wr.remote_addr = cb->remote_addr;
		cb->rdma_sq_wr.wr.sg_list->length = cb->remote_len;
		cb->rdma_sgl.lkey = krperf_rdma_rkey(cb, cb->rdma_dma_addr, !cb->read_inv);
		cb->rdma_sq_wr.wr.next = NULL;

		/* Issue RDMA Read. */
		if (cb->read_inv)
			cb->rdma_sq_wr.wr.opcode = IB_WR_RDMA_READ_WITH_INV;
		else {

			cb->rdma_sq_wr.wr.opcode = IB_WR_RDMA_READ;
			/*
			 * Immediately follow the read with a
			 * fenced LOCAL_INV.
			 */
			cb->rdma_sq_wr.wr.next = &inv;
			memset(&inv, 0, sizeof inv);
			inv.opcode = IB_WR_LOCAL_INV;
			inv.ex.invalidate_rkey = cb->reg_mr->rkey;
			inv.send_flags = IB_SEND_FENCE;
		}

		ret = ib_post_send(cb->qp, &cb->rdma_sq_wr.wr, &bad_wr);
		if (ret) {
			pr_err("post send error %d(%pe)\n", ret, ERR_PTR(ret));
			break;
		}
		cb->rdma_sq_wr.wr.next = NULL;

		DEBUG_LOG("server posted rdma read req \n");

		/* Wait for read completion */
		wait_event_interruptible(cb->sem, cb->state >= RDMA_READ_COMPLETE);
		if (cb->state != RDMA_READ_COMPLETE) {
			pr_err("wait for RDMA_READ_COMPLETE state %d\n",
				   cb->state);
			break;
		}
		DEBUG_LOG("server received read complete\n");

		/* Display data in recv buf */
		if (cb->verbose)
			printk(KERN_INFO PFX
					"server ping data (64B max): |%.64s|\n",
					cb->rdma_buf);

		/* Tell client to continue */
		if (cb->server && cb->server_invalidate) {
			cb->sq_wr.ex.invalidate_rkey = cb->remote_rkey;
			cb->sq_wr.opcode = IB_WR_SEND_WITH_INV;
			DEBUG_LOG("send-w-inv rkey 0x%x\n", cb->remote_rkey);
		}
		ret = ib_post_send(cb->qp, &cb->sq_wr, &bad_wr);
		if (ret) {
			pr_err("post send error %d(%pe)\n", ret, ERR_PTR(ret));
			break;
		}
		DEBUG_LOG("server posted go ahead\n");

		/* Wait for client's RDMA STAG/TO/Len */
		wait_event_interruptible(cb->sem, cb->state >= RDMA_WRITE_ADV);
		if (cb->state != RDMA_WRITE_ADV) {
			pr_err("wait for RDMA_WRITE_ADV state %d\n",
				   cb->state);
			break;
		}
		DEBUG_LOG("server received sink adv\n");

		/* RDMA Write echo data */
		cb->rdma_sq_wr.wr.opcode = IB_WR_RDMA_WRITE;
		cb->rdma_sq_wr.rkey = cb->remote_rkey;
		cb->rdma_sq_wr.remote_addr = cb->remote_addr;
		cb->rdma_sq_wr.wr.sg_list->length = strlen(cb->rdma_buf) + 1;
		if (cb->local_dma_lkey)
			cb->rdma_sgl.lkey = cb->pd->local_dma_lkey;
		else
			cb->rdma_sgl.lkey = krperf_rdma_rkey(cb, cb->rdma_dma_addr, 0);

		DEBUG_LOG("rdma write from lkey %x laddr %llx len %d\n",
				  cb->rdma_sq_wr.wr.sg_list->lkey,
				  (unsigned long long)cb->rdma_sq_wr.wr.sg_list->addr,
				  cb->rdma_sq_wr.wr.sg_list->length);

		ret = ib_post_send(cb->qp, &cb->rdma_sq_wr.wr, &bad_wr);
		if (ret) {
			pr_err("post send error %d(%pe)\n", ret, ERR_PTR(ret));
			break;
		}

		/* Wait for completion */
		ret = wait_event_interruptible(cb->sem, cb->state >= RDMA_WRITE_COMPLETE);
		if (cb->state != RDMA_WRITE_COMPLETE) {
			pr_err("wait for RDMA_WRITE_COMPLETE state %d\n",
				   cb->state);
			break;
		}
		DEBUG_LOG("server rdma write complete \n");

		cb->state = KRPERF_CONNECTED;

		/* Tell client to begin again */
		if (cb->server && cb->server_invalidate) {
			cb->sq_wr.ex.invalidate_rkey = cb->remote_rkey;
			cb->sq_wr.opcode = IB_WR_SEND_WITH_INV;
			DEBUG_LOG("send-w-inv rkey 0x%x\n", cb->remote_rkey);
		}
		ret = ib_post_send(cb->qp, &cb->sq_wr, &bad_wr);
		if (ret) {
			pr_err("post send error %d(%pe)\n", ret, ERR_PTR(ret));
			break;
		}
		DEBUG_LOG("server posted go ahead\n");
	}
}

int krperf_bind_server(struct krperf_cb *cb)
{
	struct sockaddr_storage sin;
	int ret;

	krperf_fill_sockaddr(&sin, cb);

	ret = rdma_bind_addr(cb->cm_id, (struct sockaddr *)&sin);
	if (ret) {
		pr_err("rdma_bind_addr error %d(%pe)\n", ret, ERR_PTR(ret));
		return ret;
	}
	DEBUG_LOG("rdma_bind_addr successful\n");

	DEBUG_LOG("rdma_listen\n");
	ret = rdma_listen(cb->cm_id, 3);
	if (ret) {
		pr_err("rdma_listen failed: %d(%pe)\n", ret, ERR_PTR(ret));
		return ret;
	}

	wait_event_interruptible(cb->sem, cb->state >= CONNECT_REQUEST);
	if (cb->state != CONNECT_REQUEST) {
		pr_err("wait for CONNECT_REQUEST state %d\n",
			cb->state);
		return -1;
	}

	if (!krperf_reg_supported(cb->child_cm_id->device))
		return -EINVAL;

	return 0;
}
