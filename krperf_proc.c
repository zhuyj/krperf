#include <linux/types.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>
#include <linux/proc_fs.h>

#include "krperf.h"
#include "krperf_proc.h"

#undef pr_fmt
#define pr_fmt(fmt) PFX fmt

extern struct mutex krperf_mutex;
extern struct list_head krperf_cbs;

/*
 * Write proc is used to start a ping client or server.
 */
static ssize_t krperf_write_proc(struct file * file, const char __user * buffer,
								 size_t count, loff_t *ppos)
{
	char *cmd;
	int rc;

	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	cmd = kzalloc(count, GFP_KERNEL);
	if (cmd == NULL) {
		pr_err("kmalloc failure\n");
		return -ENOMEM;
	}
	if (copy_from_user(cmd, buffer, count)) {
		kfree(cmd);
		return -EFAULT;
	}

	/*
	 * remove the \n.
	 */
	cmd[count - 1] = 0;
	pr_info("proc write |%s|\n", cmd);
	rc = krperf_doit(cmd);
	kfree(cmd);
	module_put(THIS_MODULE);
	if (rc)
		return rc;
	else
		return (int) count;
}

/*
 * Read proc returns stats for each device.
 */
static int krperf_read_proc(struct seq_file *seq, void *v)
{
	struct krperf_cb *cb;
	int num = 1;

	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	pr_info("proc read called...\n");
	mutex_lock(&krperf_mutex);

	list_for_each_entry(cb, &krperf_cbs, list) {
		if (cb->pd) {
			seq_printf(seq,
			     "%d-%s %lld %lld %lld %lld %lld %lld %lld %lld\n",
			     num++, cb->ib_dev->name, cb->stats.send_bytes,
			     cb->stats.send_msgs, cb->stats.recv_bytes,
			     cb->stats.recv_msgs, cb->stats.write_bytes,
			     cb->stats.write_msgs,
			     cb->stats.read_bytes,
			     cb->stats.read_msgs);
		} else {
			seq_printf(seq, "%d listen\n", num++);
		}
	}

	mutex_unlock(&krperf_mutex);
	module_put(THIS_MODULE);
	return 0;
}

static int krperf_read_open(struct inode *inode, struct file *file)
{
        return single_open(file, krperf_read_proc, inode->i_private);
}

static const struct proc_ops krperf_ops = {
	.proc_open = krperf_read_open,
	.proc_read = seq_read,
	.proc_write = krperf_write_proc,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release,
};

struct proc_dir_entry *krperf_proc_create(void)
{
	struct proc_dir_entry *krperf_proc = NULL;

	krperf_proc = proc_create("krperf", 0666, NULL, &krperf_ops);
	if (krperf_proc == NULL) {
		pr_err("cannot create /proc/krperf\n");
		return ERR_PTR(-ENOMEM);
	}
	return krperf_proc;
}
