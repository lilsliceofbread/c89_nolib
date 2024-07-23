#gcc -g game.c -nostdlib -std=c89 -e _start -lX11 -o game
gcc -g -Wall -Wpedantic game.c -std=c89 -lX11 -o game

