AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS=taskserver
taskserver_SOURCES=src/main.c src/list.c src/list.h src/mail.c src/mail.h src/base64.c src/base64.h src/util.c src/util.h src/config.c src/config.h
taskserver_LDFLAGS=-Lusr/local/lib
INCLUDES=-I/usr/local/include/
LIBS=`curl-config --libs` `mysql_config --include --libs_r` -lpthread -lm -lc -lz -O2 -Wall  
