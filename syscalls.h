#ifndef SYSCALLS_H
#define SYSCALLS_H

/* run syscalls without libc */
extern void* mmap(void* addr, unsigned long sz, int prot, int mode, int fd, unsigned long offset);
extern void  munmap(void* addr, unsigned long length);
extern void  exit(int exit_code);
extern int   gettimeofday(struct timeval* tv, struct timezone* tz);

#endif /* SYSCALLS_H */