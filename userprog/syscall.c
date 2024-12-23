#include "userprog/syscall.h"

#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "devices/timer.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "lib/kernel/stdio.h"
#include "lib/user/syscall.h"

#include <stdio.h>
#include <syscall-nr.h>

static void syscall_handler(struct intr_frame*);
static bool create_handler(char *name, unsigned size);
static int open_handler(char *name);
static void close_handler(int fd);
static bool remove_handler(const char *file_name);
static void seek_handler(int fd, unsigned position);
static unsigned tell_handler(int fd);
static int filesize_handler(int fd);
static int read_handler(int fd, void *buffer, unsigned size);
static int write_handler(int fd, const void *buffer, unsigned size);
static pid_t exec_handler(char *cmd_line);
static int wait_handler(int pid);


bool valid_pointer(void *ptr);
bool valid_string(char *str);
bool valid_buffer(void *buf, unsigned size);


void syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

bool valid_pointer(void *ptr) {
    if (ptr == NULL || !is_user_vaddr(ptr) || is_kernel_vaddr(ptr) || pagedir_get_page(thread_current()->pagedir, ptr) == NULL) return false;
    return true;
}

bool valid_string(char *str) {
    for (; is_user_vaddr(str) && pagedir_get_page(thread_current()->pagedir, str) != NULL; str++) {
        if (*str == '\0') return true;
    }
    return false;
}

bool valid_buffer(void *buf, unsigned size) {
    for (unsigned i = 0; i < size; i++) {
        if (!valid_pointer((buf + i))) return false;
    }
    return true;
}

bool create_handler(char *name, unsigned size) {
    bool created = filesys_create(name, (off_t)size);
    return created;
}

int open_handler(char *name) {
    struct file *file = filesys_open(name);
    struct thread *ct = thread_current();
    int fd;

    if (file == NULL) return -1;

    for (fd = 2; fd < 130; fd++) {
        if(ct->fd_list[fd] == NULL) {
            ct->fd_list[fd] = file;
            return fd;
        }
    }
    return -1;
}

void close_handler(int fd) {
    if (fd < 0 || fd > 130 || fd == NULL) exit_handler(-1);

    struct thread * ct = thread_current();

    if (fd != NULL) {
        if (ct->fd_list[fd] != NULL) {
            file_close(ct->fd_list[fd]);
            ct->fd_list[fd] = NULL;
        }
    }
}

bool remove_handler(const char *file_name) {
    return filesys_remove(file_name);
}

void seek_handler(int fd, unsigned position) {
    struct thread * ct = thread_current();

    if (fd < 0 || fd > 130 || fd == NULL) return -1;

    struct file *file = ct->fd_list[fd];

    if (position < 0 || file == NULL) exit_handler(-1);
    if (position <= file_length(file)) {
        file_seek(file, position);
    } else {
        file_seek(file, file_length(file));
    }
}

unsigned tell_handler(int fd) {
    struct thread *ct = thread_current();

    if (fd < 0 || fd > 130 || fd == NULL) return -1;

    struct file *file = ct->fd_list[fd];
    unsigned pos = file_tell(file);
    return pos;
}

int filesize_handler(int fd) {
    struct thread *ct = thread_current();

    if (fd < 0 || fd > 130 || fd == NULL) return -1;

    struct file *file = ct->fd_list[fd];
    int size = file_length(file);
    return size;
}

int write_handler(int fd, const void *buffer, unsigned size) {
    if (fd < 0 || fd > 130 || fd == NULL) return -1;

    if (fd == 1) {
        putbuf(buffer, size);
        return size;
    } else if (fd != NULL) {
        struct thread * ct = thread_current();
        struct file * file = ct->fd_list[fd];
        if (file == NULL) return -1;

        int written_size = file_write(file, buffer, size);
        if (written_size != 0) return written_size; 
    } else if (fd == 0) {
        exit_handler(-1);
    }
}

int read_handler(int fd, void *buffer, unsigned size) {
    struct thread *ct = thread_current();

    if (fd < 0 || fd > 130 || fd == NULL) return -1;

    if (fd == 0) {
        for (int i=0; i<size; i++) {
            buffer = *((int**)input_getc());
            buffer++;
        }
        return size;
    } else if (fd == 1){
        return -1;
    } else {
        if (ct->fd_list[fd] == NULL) return -1;
        int read_size = (int)file_read(ct->fd_list[fd], buffer, size);
        return read_size;
    }
    return -1;
}

pid_t exec_handler(char *cmd_line) {
    tid_t tid = (pid_t)process_execute(cmd_line);
    if (tid == TID_ERROR) return -1;
    return tid;
}

int exit_handler(int status) {
    for (int i=2; i<130; i++){
        close_handler(i);
    }

    struct thread *ct = thread_current();
    ct->pc->exit_status = status;

    thread_exit();
}

static void syscall_handler(struct intr_frame *f) {
    if (!valid_pointer(f->esp)) exit_handler(-1);
    if (!valid_pointer(f->esp + 4)) exit_handler(-1);
    if (!valid_pointer(f->esp + 1)) exit_handler(-1);

    int fd = -1;
    char *str = NULL;
    void *buf = NULL;
    unsigned size = 0;
    int syscall_nr = *((int*)f->esp);

    switch (syscall_nr) {
        case SYS_SLEEP: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            int millis = *(int*) (f->esp + 4);
            int64_t ticks = (int64_t)TIMER_FREQ * millis / 1000;
            timer_sleep(ticks);
            break;
        }

        case SYS_HALT: { 
            shutdown_power_off();
            break;
        }

        case SYS_CREATE: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            if (!valid_pointer(f->esp + 8)) exit_handler(-1);
            if (!valid_string(f->esp + 4)) exit_handler(-1);
            str = *(char**) (f->esp + 4);
            size = *(unsigned*) (f->esp + 8);
            if (!valid_string(str)) exit_handler(-1);
            f->eax = create_handler(str, size);
            break;
        }

        case SYS_OPEN: {
            if (!valid_pointer((f->esp + 4))) exit_handler(-1);
            if (!valid_string(f->esp + 4)) exit_handler(-1);
            str = *(char**)(f->esp + 4);
            if (!valid_string(str)) exit_handler(-1);
            f->eax = open_handler(str);
            break;
        }

        case SYS_CLOSE: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            fd = *(int*) (f->esp + 4);
            close_handler(fd);
            break;
        }

        case SYS_REMOVE: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            if (!valid_string(f->esp + 4)) exit_handler(-1);
            str = *(char**) (f->esp + 4);
            if (!valid_string(str)) exit_handler(-1);
            f->eax = remove_handler(str);
            break;
        }

        case SYS_SEEK: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            if (!valid_pointer(f->esp + 8)) exit_handler(-1);
            fd = *(int*) (f->esp + 4);
            unsigned position = *(unsigned*) (f->esp + 8);
            seek_handler(fd, position);
            break;
        }

        case SYS_TELL: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            fd = *(int*) (f->esp + 4);
            f->eax = tell_handler(fd);
            break;
        }

        case SYS_FILESIZE: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            fd = *(int*) (f->esp + 4);
            f->eax = filesize_handler(fd);
            break;
        }

        case SYS_READ: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            if (!valid_pointer(f->esp + 8)) exit_handler(-1);
            if (!valid_pointer(f->esp + 12)) exit_handler(-1);
            fd = *(int*) (f->esp + 4);
            buf = *(void**) (f->esp + 8);
            size = *(unsigned*) (f->esp + 12);
            if (!valid_buffer(buf, size)) exit_handler(-1);
            f->eax = read_handler(fd, buf, size);
            break;
        }

        case SYS_WRITE: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            if (!valid_pointer(f->esp + 8)) exit_handler(-1);
            if (!valid_pointer(f->esp + 12)) exit_handler(-1);
            fd = *(int*) (f->esp + 4);
            buf = *(void**) (f->esp + 8);
            size = *(unsigned*) (f->esp + 12);
            if (!valid_buffer(buf, size)) exit_handler(-1);
            f->eax = write_handler(fd, buf, size);
            break;
        }

        case SYS_EXIT: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            fd = *(int*) (f->esp + 4);
            f->eax = exit_handler(fd);
            break;
        }

        case SYS_EXEC: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            if (!valid_string(f->esp + 4)) exit_handler(-1);
            str = *(char**) (f->esp + 4);
            if (!valid_string(str)) exit_handler(-1);
            f->eax = process_execute(str);
            break;
        }

        case SYS_WAIT: {
            if (!valid_pointer(f->esp + 4)) exit_handler(-1);
            tid_t tid = *(int*) (f->esp + 4);
            f->eax = process_wait(tid);
            break;
        }
    }
}
