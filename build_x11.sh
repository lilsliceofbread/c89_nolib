# -fno-builtin prevents gcc optimising code into functions e.g. memset, since we don't have those
gcc game.c syscalls.S -nostdlib -std=c89 -e _start -fno-builtin -lX11 -O2 -o game


