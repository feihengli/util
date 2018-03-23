
export TOP_DIR=../../
include ../../Rules.make

TARGETS := libmpp.a

SRC :=$(wildcard *.c)
SRC += $(wildcard *.cpp)

OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

all:$(TARGETS) sensor_lib

$(TARGETS):$(OBJS)
	$(AR) rcs $@ $^
	@echo
sensor_lib:
	make -C $(SENSOR_PATH)
	@echo

%.o: %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<
	@echo
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
	@echo
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
