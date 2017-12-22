
include ../Rules.make

CFLAGS := -Wall -g -I./

SRC=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRC))

TARGET=ntpc
$(TARGET): $(OBJS)
	$(LINK) -lpthread -lm -ldl -Wl,-gc-sections -o $@ $^ $(MPI_LIBS)
	$(STRIP) $@
	cp $@ /mnt/hgfs/nfs/bpc/ -af

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET)
	rm -f *.o