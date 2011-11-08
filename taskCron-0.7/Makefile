# Makefile for taskCron
all: taskCron
CC=gcc
CFLAGS=-Wall

taskCron: taskCron.c
	$(CC) -o taskCron taskCron.c task.c $(CFLAGS)
	
clean: taskCron
	rm -f taskCron

