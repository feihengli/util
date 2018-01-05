	
include ../Rules.make

CFLAGS := -Wall -g -I./
MPI_LIBS := ../libipc.so
MPI_LIBS += ./libcharset.so.1
MPI_LIBS += ./libiconv.so.2
MPI_LIBS += ./libzbarqr.so


SRC=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRC))

TARGET=qr_scan
$(TARGET): $(OBJS)
	$(LINK) -lpthread -lm -ldl -Wl,-gc-sections -o $@ $^ $(MPI_LIBS)
	$(STRIP) $@
	cp $@ /mnt/hgfs/nfs/bpc/ -af

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET)
	rm -f *.o