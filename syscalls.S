.text

.global mmap
mmap:
    mov $9, %rax
    mov %rcx, %r10
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