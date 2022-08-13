#!/bin/sh

rm -f ./server ./server.o ./client ./client.o

gcc -o ./server server.c
gcc -o ./client client.c
