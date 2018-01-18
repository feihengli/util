
#∂‘”¶ hi_comm.h enum sample_vi_mode_e
#SENSOR_TYPE=OMNIVISION_OV9732_MIPI_720P_30FPS
#SENSOR_TYPE=OMNIVISION_OV2710_MIPI_1080P_30FPS
#SENSOR_TYPE=APTINA_AR0130_DC_720P_30FPS
#SENSOR_TYPE=SONY_IMX291_LVDS_1080P_30FPS
#SENSOR_TYPE=SONY_IMX323_DC_1080P_30FPS
#SENSOR_TYPE=SMARTSENS_SC1135_DC_960P_30FPS
#SENSOR_TYPE=APTINA_AR0237_DC_1080P_30FPS
#SENSOR_TYPE=OMNIVISION_OV4689_MIPI_4M_30FPS
#SENSOR_TYPE=SMARTSENS_SC2135_DC_1080P_30FPS
SENSOR_TYPE=SONY_IMX226_LVDS_12M_30FPS
export SENSOR_TYPE

#HIARCH=hi3518ev200
HIARCH=hi3516av200
export HIARCH

ifeq ($(HIARCH), hi3518ev200)
	CROSS_COMPILE=arm-hisiv300-linux-
	PLATFORM=$(TOP_DIR)/soc/hi3518ev200
endif

ifeq ($(HIARCH), hi3516av200)
	CROSS_COMPILE=arm-hisiv600-linux-
	PLATFORM=$(TOP_DIR)/soc/hi3516av200
endif

export PLATFORM


ifeq ($(SENSOR_TYPE), APTINA_AR0130_DC_720P_30FPS)
	SENSOR_PATH := $(PLATFORM)/sensor/ar0130
else ifeq ($(SENSOR_TYPE), OMNIVISION_OV9732_MIPI_720P_30FPS)
	SENSOR_PATH := $(PLATFORM)/sensor/omnivision_ov9732_mipi
else ifeq ($(SENSOR_TYPE), OMNIVISION_OV2710_MIPI_1080P_30FPS)
	SENSOR_PATH := $(PLATFORM)/sensor/omnivision_ov2710_mipi
else ifeq ($(SENSOR_TYPE), SONY_IMX291_LVDS_1080P_30FPS)
	SENSOR_PATH := $(PLATFORM)/sensor/sony_imx291
else ifeq ($(SENSOR_TYPE), SONY_IMX323_DC_1080P_30FPS)
	SENSOR_PATH := $(PLATFORM)/sensor/sony_imx323_i2c
else ifeq ($(SENSOR_TYPE), SMARTSENS_SC1135_DC_960P_30FPS)
	SENSOR_PATH := $(PLATFORM)/sensor/sc1135
else ifeq ($(SENSOR_TYPE), APTINA_AR0237_DC_1080P_30FPS)
	SENSOR_PATH := $(PLATFORM)/sensor/aptina_ar0237
else ifeq ($(SENSOR_TYPE), SMARTSENS_SC2135_DC_1080P_30FPS)
	SENSOR_PATH := $(PLATFORM)/sensor/sc2135
else ifeq ($(SENSOR_TYPE), SONY_IMX226_LVDS_12M_30FPS)
	SENSOR_PATH := $(PLATFORM)/sensor/sony_imx226
	SENSOR_LIB=$(PLATFORM)/sensor/sony_imx226/libsns_imx226.a
endif

export SENSOR_PATH
export SENSOR_LIB

ifeq ($(HIARCH), hi3518ev200)
	LIBS+=$(PLATFORM)/libmpp.a
	LIBS+=$(SENSOR_LIB)
	LIBS+=$(PLATFORM)/mpp_lib/libmpi.a
	LIBS+=$(PLATFORM)/mpp_lib/libive.a
	LIBS+=$(PLATFORM)/mpp_lib/libmd.a
	LIBS+=$(PLATFORM)/mpp_lib/libVoiceEngine.a
	LIBS+=$(PLATFORM)/mpp_lib/libupvqe.a
	LIBS+=$(PLATFORM)/mpp_lib/libdnvqe.a
	LIBS+=$(PLATFORM)/mpp_lib/libisp.a
	LIBS+=$(PLATFORM)/mpp_lib/lib_hiae.a
	LIBS+=$(PLATFORM)/mpp_lib/lib_hiawb.a
	LIBS+=$(PLATFORM)/mpp_lib/lib_hiaf.a
	LIBS+=$(PLATFORM)/mpp_lib/lib_hidefog.a
	LIBS+=$(wildcard $(PLATFORM)/common_lib/*.a)
endif

ifeq ($(HIARCH), hi3516av200)
	LIBS+=$(PLATFORM)/libmpp.a
	LIBS+=$(SENSOR_LIB)
	LIBS+=$(PLATFORM)/mpp_lib/libmpi.a
	LIBS+=$(PLATFORM)/mpp_lib/libive.a
	LIBS+=$(PLATFORM)/mpp_lib/libmd.a
	LIBS+=$(PLATFORM)/mpp_lib/libVoiceEngine.a
	LIBS+=$(PLATFORM)/mpp_lib/libupvqe.a
	LIBS+=$(PLATFORM)/mpp_lib/libdnvqe.a
	LIBS+=$(PLATFORM)/mpp_lib/libisp.a
	LIBS+=$(PLATFORM)/mpp_lib/lib_hiae.a
	LIBS+=$(PLATFORM)/mpp_lib/lib_hiawb.a
	LIBS+=$(PLATFORM)/mpp_lib/lib_hiaf.a
	LIBS+=$(PLATFORM)/mpp_lib/lib_hidefog.a
	LIBS+=$(wildcard $(PLATFORM)/common_lib/*.a)
endif

export LIBS

CFLAGS += -g -Wall
CFLAGS += -DSENSOR_TYPE=$(SENSOR_TYPE)
CFLAGS += -DHIARCH=$(HIARCH)
CFLAGS += -I$(TOP_DIR)
CFLAGS += -I$(TOP_DIR)/common_include
CFLAGS += -I$(PLATFORM)
CFLAGS += -I$(PLATFORM)/mpp_include
export CFLAGS

export CC=$(CROSS_COMPILE)gcc
export CPP=$(CROSS_COMPILE)g++
export CXX=$(CROSS_COMPILE)g++
export LINK=$(CROSS_COMPILE)g++
export STRIP=$(CROSS_COMPILE)strip
export AR=$(CROSS_COMPILE)ar
export RANLIB=$(CROSS_COMPILE)ranlib

