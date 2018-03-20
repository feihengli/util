
export TOP_DIR=../../
include ../../Rules.make

MPI_LIBS := $(TOP_DIR)/libipc.so
SRC=$(wildcard *.c)
SRC += $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

TARGET=test_ftpc
$(TARGET): $(OBJS)
	$(LINK) -L$(TOP_DIR) -lipc -lpthread -lm -ldl -o $@ $^
	$(STRIP) $@
	cp $@ /install/nfs/test/test_encode/ -af
	cp $(MPI_LIBS) /install/nfs/test/test_encode/ -af

%.o: %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<
	@echo
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
	@echo

clean:
	rm -f $(TARGET)
	rm -f *.o