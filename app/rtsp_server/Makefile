
include ../../Rules.make

#PWD := $(shell pwd)
PWD=.
TOP_DIR=$(PWD)/../..

ifeq ($(HIARCH), hi3518ev200)
	PLATFORM=$(TOP_DIR)/soc/hi3518ev200
else ifeq ($(HIARCH), hi3516av200)
	PLATFORM=$(TOP_DIR)/soc/hi3516av200
endif

CFLAGS := -Wall -g -I./
CFLAGS += -I$(TOP_DIR)
CFLAGS += -I$(TOP_DIR)/common_include
CFLAGS += -I$(PLATFORM)
CFLAGS += -I$(PLATFORM)/mpp_include

MPI_LIBS := $(TOP_DIR)/libipc.so
SRC=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRC))

TARGET=rtsp_server
$(TARGET): $(OBJS)
	#cp ../../lib/libipc.so ./ -af
	$(LINK) -lpthread -lm -ldl -o $@ $^ $(MPI_LIBS)
	$(STRIP) $@
	#cp $@ /install/nfs/test/test_encode/ -af
	#cp ../../lib/libipc.so /install/nfs/test/test_encode/ -af

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET)
	rm -f *.o