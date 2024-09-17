#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/parser.h>
#include <linux/proc_fs.h>
#include <linux/inet.h>
#include <linux/list.h>
#include <linux/in.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/ktime.h>
#include <linux/random.h>
#include <linux/signal.h>
#include <linux/proc_fs.h>

#include <asm/atomic.h>
#include <asm/pci.h>

#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>

#include "krperf_getopt.h"

#include "krperf_srq.h"

#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME " L" __stringify(__LINE__) ": file: %s +%d caller: %ps " fmt, __FILE__, __LINE__, __builtin_return_address(0)

void krperf_free_srq(struct krperf_cb *cb)
{
	if (!krperf_srq_valid(cb))
		return;

	ib_destroy_srq(cb->srq);
	cb->srq = NULL;
}

static void krperf_srq_event(struct ib_event *event, void *ctx)
{
	switch (event->event) {
	case IB_EVENT_SRQ_ERR:
		pr_err("event IB_EVENT_SRQ_ERR unhandled\n");
		break;
	case IB_EVENT_SRQ_LIMIT_REACHED:
		pr_err("reach SRQ_LIMIT, need to increase the value of sge\n");
		break;
	default:
		break;
	}
}

int krperf_alloc_srq(struct krperf_cb *cb)
{
	struct ib_srq_init_attr srq_attr = {
		.event_handler = krperf_srq_event,
		.srq_context = (void *)cb,
		.attr.max_wr = cb->ib_dev->attrs.max_srq_wr,
		.attr.max_sge = cb->ib_dev->attrs.max_srq_sge,
		.attr.srq_limit = cb->ib_dev->attrs.max_srq_sge / 3,
		.srq_type = IB_SRQT_BASIC,
	};

	if (!cb->use_srq)
		return 0;

	pr_info_once("SRQ in krperf is in experimental stage\n");

	if (cb->srq) {
		pr_warn("ib dev %s srq\n", cb->ib_dev->name);
		return 0;
	}

	pr_warn("ib dev %s create srq\n", cb->ib_dev->name);

	if (!cb->pd) {
		pr_warn("srq, pd NULL\n");
		WARN_ON_ONCE(1);
		return -EINVAL;
	}

	cb->srq = ib_create_srq(cb->pd, &srq_attr);

	if (IS_ERR(cb->srq)) {
		pr_debug("ib_create_srq() failed: %ld\n", PTR_ERR(cb->srq));
		return PTR_ERR(cb->srq);
	}

	return 0;
}

int krperf_ib_srq_rq_post_recv(struct krperf_cb *cb, const struct ib_recv_wr **bad_wr)
{
	int ret = 0;

	if (krperf_srq_valid(cb)) {
		ret = ib_post_srq_recv(cb->srq, &cb->rq_wr, bad_wr);
		if (ret) {
			pr_warn("ib_post_srq_recv failed: %d(%pe)\n", ret, ERR_PTR(ret));
			return ret;
		}

	} else {
		ret = ib_post_recv(cb->qp, &cb->rq_wr, bad_wr);
		if (ret) {
			pr_warn("ib__post_recv failed: %d(%pe)\n", ret, ERR_PTR(ret));
			return ret;
		}
	}

	return 0;
}
