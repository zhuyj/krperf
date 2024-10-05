#undef TRACE_SYSTEM
#define TRACE_SYSTEM krperf
#if !defined(_TRACE_KRPERF_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_KRPERF_H

#include <linux/tracepoint.h>

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
#endif /* _TRACE_KRPERF_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE krperf_trace

/* This part must be outside protection */
#include <trace/define_trace.h>
