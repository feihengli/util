
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

%.o: %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
	
%.d:%.c
	@set -e; rm -f $@; $(CC) -MM $< $(CFLAGS) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
%.d:%.cpp
	@set -e; rm -f $@; $(CXX) -MM $< $(CFLAGS) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(OBJS:.o=.d)

clean:
	make -C $(SENSOR_PATH) clean
	rm -r $(TARGETS);rm *.o;rm -f *.d
