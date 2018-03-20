
export TOP_DIR=../../
include ../../Rules.make

MPI_LIBS := $(TOP_DIR)/libipc.so
SRC=$(wildcard *.c)
SRC += $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

TARGET=cuftpd
$(TARGET): $(OBJS)
	#$(LINK) -L$(TOP_DIR) -lipc -lpthread -lm -ldl -o $@ $^
	$(LINK) -lpthread -lm -ldl -o $@ $^
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