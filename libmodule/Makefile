

include ../Rules.make

TARGETS := libmodule.a
PRINTF := @printf "\033[31;1m\t%s\033[0m\n"

SRC :=$(wildcard *.c)
SRC += $(wildcard *.cpp)

OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

CXXFLAGS := -Wall -I./
CXXFLAGS += -I../libbase
CXXFLAGS += -I../libdigest
CXXFLAGS += -I../libmd5
CXXFLAGS += -I../libtinyxml
CXXFLAGS += -I../opensource
CXXFLAGS += -I../soc/hi3518ev200

all:$(TARGETS)

$(TARGETS):$(OBJS)
	$(PRINTF) "AR  $@";$(AR) rcs $@ $^

%.o: %.cpp
	$(PRINTF) "$(CXX) $@";$(CXX) -c $(CXXFLAGS) -o $@ $<

%.o: %.c
	$(PRINTF) "$(CC) $@";$(CC) -c $(CXXFLAGS) -o $@ $<

clean:
	$(PRINTF) "RM  $(TARGETS)";rm -r $(TARGETS);rm *.o
