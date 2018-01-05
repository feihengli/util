

include ../Rules.make

TARGETS := libbase.a
PRINTF := @printf "\033[31;1m\t%s\033[0m\n"

SRC :=$(wildcard *.c)
SRC += $(wildcard *.cpp)

OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

CXXFLAGS := -Wall -I./

all:$(TARGETS)

$(TARGETS):$(OBJS)
	$(PRINTF) "AR  $@";$(AR) rcs $@ $^

%.o: %.cpp
	$(PRINTF) "$(CXX) $@";$(CXX) -c $(CXXFLAGS) -o $@ $<

%.o: %.c
	$(PRINTF) "$(CC) $@";$(CC) -c $(CXXFLAGS) -o $@ $<

clean:
	$(PRINTF) "RM  $(TARGETS)";rm -r $(TARGETS);rm *.o
