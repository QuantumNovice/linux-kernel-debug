#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/sched/signal.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/dcache.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("List processes with binary paths");
MODULE_AUTHOR("You");

static struct dentry *debug_entry;

/* convert scheduler state to human-readable text */
static const char *state_to_text(unsigned char s)
{
    switch (s) {
    case TASK_RUNNING:         return "RUNNING";
    case TASK_INTERRUPTIBLE:   return "SLEEPI";
    case TASK_UNINTERRUPTIBLE: return "SLEEPU";
    case TASK_DEAD:            return "DEAD";
    default:                   return "OTHER";
    }
}

/* extract full executable path */
static void get_binary_path(struct task_struct *task, char *buf, int buflen)
{
    struct mm_struct *mm;
    struct file *exe;
    char *tmp;

    buf[0] = 0;

    mm = get_task_mm(task);
    if (!mm)
        return;

    exe = mm->exe_file;
    if (!exe) {
        mmput(mm);
        return;
    }

    tmp = d_path(&exe->f_path, buf, buflen);

    if (!IS_ERR(tmp))
        strscpy(buf, tmp, buflen);
    else
        buf[0] = 0;

    mmput(mm);
}

/* main listing function */
static int show_fn(struct seq_file *m, void *v)
{
    struct task_struct *task;
    char pathbuf[512];

    seq_puts(m, "PID   PPID   STATE       NAME          BINARY\n");

    for_each_process(task) {

        unsigned char st = task_state_to_char(task);
        get_binary_path(task, pathbuf, sizeof(pathbuf));

        seq_printf(m,
            "%-5d %-5d %-10s %-14s %s\n",
            task->pid,
            task_ppid_nr(task),
            state_to_text(st),
            task->comm,
            pathbuf[0] ? pathbuf : "(none)"
        );
    }

    return 0; /* important */
}

static int open_fn(struct inode *inode, struct file *file)
{
    return single_open(file, show_fn, NULL);
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = open_fn,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release
};

static int __init mod_init(void)
{
    debug_entry = debugfs_create_file("proclist", 0444, NULL, NULL, &fops);

    if (!debug_entry) {
        pr_err("proclist: could not create debugfs file\n");
        return -ENOMEM;
    }

    pr_info("proclist: loaded\n");
    return 0;
}

static void __exit mod_exit(void)
{
    debugfs_remove(debug_entry);
    pr_info("proclist: unloaded\n");
}

module_init(mod_init);
module_exit(mod_exit);
