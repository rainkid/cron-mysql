AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS=taskserver
taskserver_SOURCES=src/task.c src/task.h src/list.c src/list.h src/mail.c src/mail.h src/base64.c src/base64.h src/util.c src/util.h src/config.c src/config.h
taskserver_LDFLAGS=-Lusr/local/lib
INCLUDES=-I/usr/local/include/
LIBS=-lcurl -lmysqlclient -lrt -lpthread -Wl -lm -lc -lz -O2
