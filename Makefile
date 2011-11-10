# Makefile for cron 
CC=gcc
CFLAGS=-I/usr/include/mysql -L/usr/lib/mysql -lmysqlclient -L/usr/local/curl/lib/ -lcurl -I/usr/local/curl/include/ -g -Wall -lrt -lpthread -lm -lc -lz -fno-builtin-strlen -Bstatic -Bdynamic

cron: main.c
	$(CC) -o cron main.c lib/task.c lib/tool.c $(CFLAGS)
	
clean: cron 
	rm -f cron 

