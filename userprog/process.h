#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

struct thread_data {
  char *cl_copy;
  struct thread *parent;
};

tid_t process_execute(const char* cmd_line);
int process_wait(tid_t);
void process_exit(void);
void process_activate(void);

#endif /* userprog/process.h */
