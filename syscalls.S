.text

# %rax must contain the number of the syscall
# the rest of the arguments to the syscall are the same as the normal function parameter registers so no need to list them
.global mmap
mmap:
    mov $9, %rax
    mov %rcx, %r10 # don't know exactly why this is required, but it works
    syscall
    ret

.global munmap
munmap:
    mov $11, %rax
    syscall
    ret

.global exit
exit:
    mov $60, %rax
    syscall
    ret

.global gettimeofday
gettimeofday:
    mov $96, %rax
    syscall
    ret