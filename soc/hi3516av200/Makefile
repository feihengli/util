
export TOP_DIR=../../
include ../../Rules.make

TARGETS := libmpp.a

SRC :=$(wildcard *.c)
SRC += $(wildcard *.cpp)

OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

all:$(TARGETS) sensor_lib

$(TARGETS):$(OBJS)
	$(AR) rcs $@ $^
sensor_lib:
	make -C $(SENSOR_PATH)
	cp $(SENSOR_PATH)/*.a ./libsns.a -af

%.o: %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	make -C $(SENSOR_PATH) clean
	rm -r $(TARGETS);rm *.o *.a
