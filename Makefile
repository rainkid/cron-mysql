# Makefile for cron 
CC=gcc
LIBLOAD=-L/usr/lib/mysql -lmysqlclient -L/usr/local/curl/lib/ -lcurl -lrt -lpthread -lm -lc -lz
INCLUDE= -I/usr/include/mysql -I/usr/local/curl/include/
CFLAGS= -g -Wall -fno-builtin-strlen -Bstatic -Bdynamic

task: main.c
	$(CC) -o task main.c library/task.c library/tool.c library/base64.c library/send_mail.c $(LIBLOAD) $(INCLUDE) $(CFLAGS)

clean: task 
	rm -f task 
