#gcc -g -O2 game.c -nostdlib -std=c89 -e _start -lX11 -o game
gcc -g -O2 -Wall -Wpedantic game.c -std=c89 -lX11 -o game

