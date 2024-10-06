#undef TRACE_SYSTEM
#define TRACE_SYSTEM krperf
#if !defined(_TRACE_KRPERF_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_KRPERF_H

#include <linux/tracepoint.h>

struct krperf_cb;

TRACE_EVENT(krperf_debug,
    TP_PROTO(char *file, const char *func, int line, char *msg),
    TP_ARGS(file, func, line, msg),
    TP_STRUCT__entry(
        __field(char *, file)
        __field(const char *, func)
        __field(int, line)
        __field(char *, msg)
    ),
    TP_fast_assign(
        __entry->file = file;
        __entry->func = func;
        __entry->line = line;
        __entry->msg = msg;
    ),

    TP_printk("file: %s +%d func:%s: %s", __entry->file, __entry->line, __entry->func, __entry->msg)
);

#define T_trace_krperf_debug(s) trace_krperf_debug(__FILE__, __func__, __LINE__, s)

TRACE_EVENT(krperf_srv_recv,
    TP_PROTO(char *file, const char *func, int line, struct krperf_cb *cb),
    TP_ARGS(file, func, line, cb),
    TP_STRUCT__entry(
        __field(char *, file)
        __field(const char *, func)
        __field(int, line)
        __field(unsigned int, rkey)
        __field(unsigned long long, remote_addr)
        __field(unsigned int, len)
    ),
    TP_fast_assign(
        __entry->file = file;
        __entry->func = func;
        __entry->line = line;
        __entry->rkey = cb->remote_rkey;
        __entry->remote_addr = (unsigned long long)cb->remote_addr;
        __entry->len = cb->remote_len;
    ),

    TP_printk("file: %s +%d func:%s: Received rkey %x addr %llx len %d from peer",
                 __entry->file, __entry->line, __entry->func, __entry->rkey,
                 __entry->remote_addr, __entry->len)
);

#define T_trace_krperf_srv_recv(cb) trace_krperf_srv_recv(__FILE__, __func__, __LINE__, cb)

TRACE_EVENT(krperf_remote_rkey,
    TP_PROTO(char *file, const char *func, int line, unsigned int remote_rkey),
    TP_ARGS(file, func, line, remote_rkey),
    TP_STRUCT__entry(
        __field(char *, file)
        __field(const char *, func)
        __field(int, line)
        __field(unsigned int, remote_rkey)
    ),
    TP_fast_assign(
        __entry->file = file;
        __entry->func = func;
        __entry->line = line;
        __entry->remote_rkey = remote_rkey;
    ),

    TP_printk("file: %s +%d func:%s: send-w-inv rkey 0x%x", __entry->file, __entry->line, __entry->func, __entry->remote_rkey)
);

#define T_trace_krperf_remote_rkey(remote_rkey) trace_krperf_remote_rkey(__FILE__, __func__, __LINE__, remote_rkey)

TRACE_EVENT(krperf_lkey,
    TP_PROTO(char *file, const char *func, int line, struct krperf_cb *cb),
    TP_ARGS(file, func, line, cb),
    TP_STRUCT__entry(
        __field(char *, file)
        __field(const char *, func)
        __field(int, line)
        __field(unsigned int, lkey)
        __field(unsigned long long, addr)
        __field(unsigned int, len)
    ),
    TP_fast_assign(
        __entry->file = file;
        __entry->func = func;
        __entry->line = line;
        __entry->lkey = cb->rdma_sq_wr.wr.sg_list->lkey;
        __entry->addr = (unsigned long long)cb->rdma_sq_wr.wr.sg_list->addr;
        __entry->len = cb->rdma_sq_wr.wr.sg_list->length;
    ),

    TP_printk("file: %s +%d func:%s: rdma write from lkey %x laddr %llx len %d",
            __entry->file, __entry->line, __entry->func, __entry->lkey, __entry->addr, __entry->len)
);

#define T_trace_krperf_lkey(cb) trace_krperf_lkey(__FILE__, __func__, __LINE__, cb)

#endif /* _TRACE_KRPERF_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE krperf_trace

/* This part must be outside protection */
#include <trace/define_trace.h>
