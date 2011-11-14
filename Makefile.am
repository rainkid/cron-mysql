AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS=task
task_SOURCES=main.c library/task.c library/task.h library/send_mail.c library/send_mail.h library/base64.c library/base64.h library/tool.c library/tool.h
task_LDFLAGS=-Lusr/local/lib
INCLUDES=-I/usr/local/include/
LIBS=-lcurl -lmysqlclient -lrt -lpthread -lm -lc -lz
 
