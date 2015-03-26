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

// on module load
static int __init load_module(void) {
  return 0;
}

// on module unload
static void __exit cleanup(void) {
  return;
}

module_init(load_module);
module_exit(cleanup);
