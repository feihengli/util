# Makefile

export TOP_DIR=./
include Rules.make

TARGET=libipc.so

SRC += $(wildcard *.c)
SRC += $(wildcard *.cpp)
OBJS = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

$(TARGET): $(OBJS)
	make -C $(PLATFORM)
	@echo
	$(LINK) -shared -o $@ $^ $(LIBS)
	$(STRIP) --strip-debug --strip-unneeded $@

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
	rm -f $(TARGET)
	rm -f *.o
	rm -f *.d
	make -C $(PLATFORM) clean
	@echo
	
