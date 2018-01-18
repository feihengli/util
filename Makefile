# Hisilicon Hi3516 sample Makefile

export TOP_DIR=./
include Rules.make

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
	rm -f $(TARGET)
	rm -f *.o
	make -C $(PLATFORM) clean
	@echo
	
