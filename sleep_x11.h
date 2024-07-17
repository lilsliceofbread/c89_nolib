#ifndef SLEEP_X11_H
#define SLEEP_X11_H

#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>

// TODO: REMOVE OTHER STDLIB HEADERS, create own types e.g. time_t, timespec
// * standard library implementation of sleep with linux kernel syscalls (idk how the hell else to do this)
/* We are going to use the `nanosleep' syscall of the kernel.  But the
   kernel does not implement the stupid SysV SIGCHLD vs. SIG_IGN
   behaviour for this syscall.  Therefore we have to emulate it here.  */
unsigned int sleep(unsigned int seconds)
{
    // 32 bit num with top bit as 0
    const unsigned int max = (unsigned int)(((unsigned long int)(~((time_t)0))) >> 1);
    struct timespec ts;
    sigset_t set, oset;
    unsigned int result;

    ts.tv_sec = 0;
    ts.tv_nsec = 0;
again:
    if(sizeof(ts.tv_sec) <= sizeof(seconds))
    {
        /* Since SECONDS is unsigned assigning the value to .tv_sec can
       overflow it.  In this case we have to wait in steps.  */
        ts.tv_sec += MIN(seconds, max);
        seconds -= (unsigned int)ts.tv_sec;
    }
    else
    {
        ts.tv_sec = (time_t)seconds;
        seconds = 0;
    }

    /* Linux will wake up the system call, nanosleep, when SIGCHLD
       arrives even if SIGCHLD is ignored.  We have to deal with it
       in libc.  We block SIGCHLD first.  */
    __sigemptyset(&set);
    __sigaddset(&set, SIGCHLD);
    if(__sigprocmask(SIG_BLOCK, &set, &oset))
        return -1;

    /* If SIGCHLD is already blocked, we don't have to do anything.  */
    if(!__sigismember(&oset, SIGCHLD))
    {
        int saved_errno;
        struct sigaction oact;

        __sigemptyset(&set);
        __sigaddset(&set, SIGCHLD);

        /* We get the signal handler for SIGCHLD.  */
        if(__sigaction(SIGCHLD, (struct sigaction *)NULL, &oact) < 0)
        {
            saved_errno = errno;
            /* Restore the original signal mask.  */
            (void)__sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
            __set_errno(saved_errno);
            return -1;
        }

        /* Note the sleep() is a cancellation point.  But since we call
       nanosleep() which itself is a cancellation point we do not
       have to do anything here.  */
        if(oact.sa_handler == SIG_IGN)
        {
            //__libc_cleanup_push (cl, &oset);

            /* We should leave SIGCHLD blocked.  */
            while (1)
            {
                result = __nanosleep(&ts, &ts);

                if(result != 0 || seconds == 0)
                    break;

                if(sizeof(ts.tv_sec) <= sizeof(seconds))
                {
                    ts.tv_sec = MIN(seconds, max);
                    seconds -= (unsigned int)ts.tv_nsec;
                }
            }

            //__libc_cleanup_pop (0);

            saved_errno = errno;
            /* Restore the original signal mask.  */
            (void)__sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
            __set_errno(saved_errno);

            goto out;
        }

        /* We should unblock SIGCHLD.  Restore the original signal mask.  */
        (void)__sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
    }

    result = __nanosleep(&ts, &ts);
    if(result == 0 && seconds != 0)
        goto again;

out:
    if(result != 0)
        /* Round remaining time.  */
        result = seconds + (unsigned int)ts.tv_sec + (ts.tv_nsec >= 500000000L);

    return result;
}

#endif /* SLEEP_X11_H */