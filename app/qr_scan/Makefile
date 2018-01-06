
include ../../Rules.make

CFLAGS := -Wall -g -I./
CFLAGS += -I../../
CFLAGS += -I../../opensource
CFLAGS += -I../../libbase
CFLAGS += -I../../libdigest
CFLAGS += -I../../libmd5
CFLAGS += -I../../libmodule
CFLAGS += -I../../libtinyxml

ifeq ($(HIARCH), hi3518ev200)
	CFLAGS += -I../../soc/hi3518ev200
endif

MPI_LIBS := ../../lib/libipc.so
MPI_LIBS += ./libcharset.so.1
MPI_LIBS += ./libiconv.so.2
MPI_LIBS += ./libzbarqr.so


SRC=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRC))

TARGET=qr_scan
$(TARGET): $(OBJS)
	$(LINK) -lpthread -lm -ldl -Wl,-gc-sections -o $@ $^ $(MPI_LIBS)
	$(STRIP) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET)
	rm -f *.o