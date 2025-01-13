/*
 * lifted from krperf.c
 */
#include <linux/kernel.h>
#include <linux/string.h>

#include <asm/errno.h>
#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>
#include <linux/inet.h>

#include "krperf_getopt.h"

#undef pr_fmt
#define pr_fmt(fmt) PFX fmt
/**
 *	krperf_getopt - option parser
 *	@caller: name of the caller, for error messages
 *	@options: the options string
 *	@opts: an array of &struct option entries controlling parser operations
 *	@optopt: output; will contain the current option
 *	@optarg: output; will contain the value (if one exists)
 *	@flag: output; may be NULL; should point to a long for or'ing flags
 *	@value: output; may be NULL; will be overwritten with the integer value
 *		of the current argument.
 *
 *	Helper to parse options on the format used by mount ("a=b,c=d,e,f").
 *	Returns opts->val if a matching entry in the 'opts' array is found,
 *	0 when no more tokens are found, -1 if an error is encountered.
 */
static int krperf_getopt(const char *caller, char **options,
		  const struct krperf_option *opts, char **optopt,
		  char **optarg, unsigned long *value)
{
	char *token;
	char *val;

	do {
		if ((token = strsep(options, ",")) == NULL)
			return 0;
	} while (*token == '\0');

	if (optopt)
		*optopt = token;

	if ((val = strchr (token, '=')) != NULL) {
		*val++ = 0;
	}

	*optarg = val;
	for (; opts->name; opts++) {
		if (!strcmp(opts->name, token)) {
			if (!val) {
				if (opts->has_arg & OPT_NOPARAM) {
					return opts->val;
				}

				pr_info("%s: the %s option requires an argument\n", caller, token);
				return -EINVAL;
			}

			if (opts->has_arg & OPT_INT) {
				char* v;

				*value = simple_strtoul(val, &v, 0);
				if (!*v) {
					return opts->val;
				}
				pr_info("%s: invalid numeric value in %s=%s\n", caller, token, val);
				return -EDOM;
			}

			if (opts->has_arg & OPT_STRING) {
				return opts->val;
			}
			pr_info("%s: unexpected argument %s to the %s option\n", caller, val, token);
			return -EINVAL;
		}
	}

	pr_info("%s: Unrecognized option %s\n", caller, token);
	return -EOPNOTSUPP;
}

int krperf_parse(char *cmd, struct krperf_cb *cb)
{
	unsigned long optint;
	char *optarg;
	int ret = 0;
	char *scope;
	int op;

	while ((op = krperf_getopt("krperf", &cmd, krperf_opts, NULL, &optarg,
			      &optint)) != 0) {
		switch (op) {
		case 'a':
			cb->addr_str = optarg;
			in4_pton(optarg, -1, cb->addr, -1, NULL);
			cb->addr_type = AF_INET;
			pr_info("ipaddr (%s)\n", optarg);
			break;
		case 'A':
			cb->addr_str = optarg;
			scope = strstr(optarg, "%");
			if (scope != NULL) {
				*scope++ = 0;
				strncpy(cb->ip6_ndev_name, scope,
					sizeof(cb->ip6_ndev_name));
				/* force zero-termination */
				cb->ip6_ndev_name[
				        sizeof(cb->ip6_ndev_name) - 1] = 0;
			}
			in6_pton(optarg, -1, cb->addr, -1, NULL);
			cb->addr_type = AF_INET6;
			pr_info("ipv6addr (%s)\n", optarg);
			break;
		case 'p':
			cb->port = htons(optint);
			pr_info("port %d\n", (int)optint);
			break;
		case 'P':
			cb->poll = 1;
			pr_info("poll\n");
			break;
		case 's':
			cb->server = 1;
			pr_info("server\n");
			break;
		case 'c':
			cb->server = 0;
			pr_info("client\n");
			break;
		case 'S':
			cb->size = optint;
			if ((cb->size < 1) || (cb->size > RPERF_BUFSIZE)) {
				pr_err("Invalid size %d (valid range is 1 to %d)\n",
				       cb->size, RPERF_BUFSIZE);
				ret = EINVAL;
			} else {
				pr_info("size %d\n", (int)optint);
			}
			break;
		case 'C':
			cb->count = optint;
			if (cb->count < 0) {
				pr_err("Invalid count %d\n", cb->count);
				ret = EINVAL;
			} else {
				pr_info("count %d\n", (int) cb->count);
			}
			break;
		case 'v':
			cb->verbose++;
			pr_info("verbose\n");
			break;
		case 'V':
			cb->validate++;
			pr_info("validate data\n");
			break;
		case 'd':
			cb->duplex++;
			break;
		case 'I':
			cb->server_invalidate = 1;
			break;
		case 't':
			cb->tos = optint;
			pr_info("type of service, tos=%d\n", (int) cb->tos);
			break;
		case 'T':
			cb->txdepth = optint;
			pr_info("txdepth %d\n", (int) cb->txdepth);
			break;
		case 'Z':
			cb->local_dma_lkey = 1;
			pr_info("using local dma lkey\n");
			break;
		case 'R':
			cb->read_inv = 1;
			pr_info("using read-with-inv\n");
			break;
		case 'f':
			cb->frtest = 1;
			pr_info("fast-reg test!\n");
			break;
		case 'q':
			cb->use_srq = true;
			cb->srq = NULL;
			break;
		default:
			pr_err("unknown opt %s\n", optarg);
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}
