

include ../../Rules.make

ifeq ($(SENSOR_TYPE), APTINA_AR0130_DC_720P_30FPS)
	SENSOR_PATH := ./sensor/ar0130
else ifeq ($(SENSOR_TYPE), OMNIVISION_OV9732_MIPI_720P_30FPS)
	SENSOR_PATH := ./sensor/omnivision_ov9732_mipi
else ifeq ($(SENSOR_TYPE), OMNIVISION_OV2710_MIPI_1080P_30FPS)
	SENSOR_PATH := ./sensor/omnivision_ov2710_mipi
else ifeq ($(SENSOR_TYPE), SONY_IMX291_LVDS_1080P_30FPS)
	SENSOR_PATH := ./sensor/sony_imx291
else ifeq ($(SENSOR_TYPE), SONY_IMX323_DC_1080P_30FPS)
	SENSOR_PATH := ./sensor/sony_imx323_i2c
else ifeq ($(SENSOR_TYPE), SMARTSENS_SC1135_DC_960P_30FPS)
	SENSOR_PATH := ./sensor/sc1135
else ifeq ($(SENSOR_TYPE), APTINA_AR0237_DC_1080P_30FPS)
	SENSOR_PATH := ./sensor/aptina_ar0237
else ifeq ($(SENSOR_TYPE), SMARTSENS_SC2135_DC_1080P_30FPS)
	SENSOR_PATH := ./sensor/sc2135
endif

TARGETS := libmpp.a
PRINTF := @printf "\033[31;1m\t%s\033[0m\n"

SRC :=$(wildcard *.c)
SRC += $(wildcard *.cpp)

OBJS := $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

CXXFLAGS := -Wall -I./
CXXFLAGS += -DSENSOR_TYPE=$(SENSOR_TYPE)
CXXFLAGS += -I./mpp_include
CXXFLAGS += -I../../
CXXFLAGS += -I../../libbase

all:$(TARGETS) sensor_lib

$(TARGETS):$(OBJS)
	$(PRINTF) "AR  $@";$(AR) rcs $@ $^
sensor_lib:
	make -C $(SENSOR_PATH)
	cp $(SENSOR_PATH)/*.a ./ -af

%.o: %.cpp
	$(PRINTF) "$(CXX) $@";$(CXX) -c $(CXXFLAGS) -o $@ $<

%.o: %.c
	$(PRINTF) "$(CC) $@";$(CC) -c $(CXXFLAGS) -o $@ $<

clean:
	make -C $(SENSOR_PATH) clean
	$(PRINTF) "RM  $(TARGETS)";rm -r $(TARGETS);rm *.o *.a
