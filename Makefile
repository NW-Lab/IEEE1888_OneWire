CC=gcc
COMMON_OBJS   = ieee1888_XMLgenerator.o ieee1888_XMLparser.o ieee1888_client.o ieee1888_object_factory.o ieee1888_server.o ieee1888_util.o
LFLAG = -lpthread

all:	ieee1888_1wire

ieee1888_1wire: ieee1888_1wire.o $(COMMON_OBJS)
		$(CC) $(COMMON_OBJS) ieee1888_1wire.o -o ieee1888_1wire $(LFLAG)

install: ieee1888_1wire
	install -D ieee1888_1wire	/usr/local/bin/ieee1888_1wire


clean: 
	rm -f *.o ieee1888_1wire *~
