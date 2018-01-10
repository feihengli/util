

include ../Rules.make

TARGETS := libmd5.a
PRINTF := @printf "\033[31;1m\t%s\033[0m\n"

SRC :=$(wildcard *.c)
SRC += $(wildcard *.cpp)

OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

CXXFLAGS := -Wall -I./

all:$(TARGETS)

$(TARGETS):$(OBJS)
	$(PRINTF) "$(AR)  $@";$(AR) rcs $@ $^
	$(PRINTF) "RANLIB  $@";$(RANLIB) $@

%.o: %.cpp
	$(PRINTF) "$(CXX) $@";$(CXX) -c $(CXXFLAGS) -o $@ $<

%.o: %.c
	$(PRINTF) "$(CC) $@";$(CC) -c $(CXXFLAGS) -o $@ $<

clean:
	$(PRINTF) "RM  $(TARGETS)";rm -r $(TARGETS);rm *.o
