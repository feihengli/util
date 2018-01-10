
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

ifeq ($(HIARCH), hi3516av200)
	CFLAGS += -I../../soc/hi3516av200
endif

MPI_LIBS := libipc.so
#MPI_LIBS := ../../lib/libipc.a
SRC=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRC))

TARGET=rtsp_server
$(TARGET): $(OBJS)
	cp ../../lib/libipc.so ./ -af
	$(LINK) -lpthread -lm -ldl -o $@ $^ $(MPI_LIBS)
	$(STRIP) $@
	cp $@ /install/nfs/test/test_encode/ -af
	cp ../../lib/libipc.so /install/nfs/test/test_encode/ -af

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET)
	rm -f *.o