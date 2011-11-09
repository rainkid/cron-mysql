# Makefile for cron 
all: cron 
CC=gcc
CFLAGS=-I/usr/include/mysql -L/usr/lib/mysql -lmysqlclient -L/usr/local/curl/lib/ -lcurl -g -Wall -lm -lc -lz -fno-builtin-strlen -Bstatic -Bdynamic

cron: main.c
	$(CC) -o cron main.c lib/task.c lib/tool.c $(CFLAGS)
	
clean: cron 
	rm -f cron 

