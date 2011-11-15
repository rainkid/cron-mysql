AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS=ctask
ctask_SOURCES=main.c library/task.c library/task.h library/mail.c library/mail.h library/base64.c library/base64.h library/tool.c library/tool.h
ctask_LDFLAGS=-Lusr/local/lib
INCLUDES=-I/usr/local/include/
LIBS=-lcurl -lmysqlclient -lrt -lpthread -Wl -lm -lc -lz -O2 
 
