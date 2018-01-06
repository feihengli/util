
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
SRC=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRC))

TARGET=upgrade_test
$(TARGET): $(OBJS)
	$(LINK) -lpthread -lm -ldl -o $@ $^ $(MPI_LIBS)
	$(STRIP) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET)
	rm -f *.o