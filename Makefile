# Hisilicon Hi3516 sample Makefile

include Rules.make

#PWD := $(shell pwd)
PWD := .

ifeq ($(HIARCH), hi3518ev200)
	PLATFORM=$(PWD)/soc/hi3518ev200
else ifeq ($(HIARCH), hi3516av200)
	PLATFORM=$(PWD)/soc/hi3516av200
endif


CFLAGS+=-Wall -g -fPIC
CFLAGS+=-I$(PWD)
CFLAGS+=-I$(PWD)/common_include
CFLAGS+=-I$(PLATFORM)
CFLAGS+=-I$(PLATFORM)/mpp_include/

LIBS+=$(PLATFORM)/libmpp.a
LIBS+=$(PLATFORM)/libsns.a
LIBS+=$(PLATFORM)/mpp_lib/libmpi.a
LIBS+=$(PLATFORM)/libmpp.a
LIBS+=$(PLATFORM)/mpp_lib/libive.a
LIBS+=$(PLATFORM)/mpp_lib/libmd.a
LIBS+=$(PLATFORM)/libmpp.a
LIBS+=$(PLATFORM)/mpp_lib/libVoiceEngine.a
LIBS+=$(PLATFORM)/libmpp.a
LIBS+=$(PLATFORM)/mpp_lib/libupvqe.a
LIBS+=$(PLATFORM)/mpp_lib/libdnvqe.a
LIBS+=$(PLATFORM)/mpp_lib/libisp.a
LIBS+=$(PLATFORM)/libmpp.a
LIBS+=$(PLATFORM)/mpp_lib/lib_hiae.a
LIBS+=$(PLATFORM)/mpp_lib/lib_hiawb.a
LIBS+=$(PLATFORM)/libmpp.a
LIBS+=$(PLATFORM)/mpp_lib/lib_hiaf.a
LIBS+=$(PLATFORM)/mpp_lib/lib_hidefog.a


#common lib
LIBS+=$(wildcard $(PLATFORM)/common_lib/*.a)

SRC=$(wildcard *.c)
SRC += $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))
TARGET=libipc.so

$(TARGET): $(OBJS)
	make -C $(PLATFORM)
	@echo
	$(LINK) -shared -o $@ $^ $(LIBS)
	$(STRIP) $@

%.o: %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<
	@echo
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
	@echo

clean:
	make -C $(PLATFORM) clean
	@echo
	rm -f $(TARGET)
	rm -f *.o
