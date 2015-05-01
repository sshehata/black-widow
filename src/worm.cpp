/*
 * =====================================================================================
 *
 *       Filename:  worm.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/28/2015 02:34:33 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <cstdio>

using namespace std;

int main() {
  char message[100];
  strcpy(message, "simon says: ");
  int i = sizeof("simon says: ") - 1;
  message[i++] = 2;
  write(0, message, i);
  printf("lol");
  execl("/bin/bash", "/bin/bash", (char*)NULL);
  return 0;
}
