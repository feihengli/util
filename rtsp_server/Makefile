
include ../Rules.make

CFLAGS := -Wall -g -I./
MPI_LIBS := ../libipc.so
SRC=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRC))

TARGET=rtsp_server
$(TARGET): $(OBJS)
	$(LINK) -lpthread -lm -ldl -o $@ $^ $(MPI_LIBS)
	$(STRIP) $@
	cp $@ /mnt/hgfs/nfs/bpc/ -af

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET)
	rm -f *.o