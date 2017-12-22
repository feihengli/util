
CROSS := arm-hisiv300-linux-

export CC = $(CROSS)gcc
export CPP = $(CROSS)g++
export LINK = $(CROSS)g++
export STRIP = $(CROSS)strip
export AR = $(CROSS)ar

CFLAGS := -Wall -g -I./
MPI_LIBS := ../libipc.so
SRC=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRC))

TARGET=test_encode
$(TARGET): $(OBJS)
	$(LINK) -lpthread -lm -ldl -o $@ $^ $(MPI_LIBS)
	$(STRIP) $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET)
	rm -f *.o