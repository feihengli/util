
#∂‘”¶ hi_comm.h enum sample_vi_mode_e
#SENSOR_INVALID
#OMNIVISION_OV9732_MIPI_720P_30FPS
#OMNIVISION_OV2710_MIPI_1080P_30FPS
#APTINA_AR0130_DC_720P_30FPS
#SONY_IMX291_LVDS_1080P_30FPS
#SONY_IMX323_DC_1080P_30FPS
#SMARTSENS_SC1135_DC_960P_30FPS
#APTINA_AR0237_DC_1080P_30FPS
#OMNIVISION_OV4689_MIPI_4M_30FPS
#SMARTSENS_SC2135_DC_1080P_30FPS
#SONY_IMX226_LVDS_12M_30FPS
#SONY_IMX274_MIPI_8M_30FPS
#SONY_IMX327_MIPI_1080P_30FPS
#SENSOR_VIRTUAL

export SENSOR_TYPE0=SONY_IMX327_MIPI_1080P_30FPS
export SENSOR_TYPE1=SENSOR_VIRTUAL

HIARCH=hi3516av200
export HIARCH

CROSS_COMPILE=arm-hisiv600-linux-
PLATFORM=$(TOP_DIR)/soc/hi3516av200
export PLATFORM

CFLAGS=
ifeq ($(SENSOR_TYPE0)__$(SENSOR_TYPE1), SONY_IMX274_MIPI_8M_30FPS__SENSOR_INVALID)
	CFLAGS += -DSNS_IMX274_MIPI_SINGLE
else ifeq ($(SENSOR_TYPE0)__$(SENSOR_TYPE1), SONY_IMX327_MIPI_1080P_30FPS__SENSOR_INVALID)
	CFLAGS += -DSNS_IMX327_MIPI_SINGLE
else ifeq ($(SENSOR_TYPE0)__$(SENSOR_TYPE1), SONY_IMX327_MIPI_1080P_30FPS__SENSOR_VIRTUAL)
	CFLAGS += -DSNS_IMX327_MIPI__SENSOR_VIRTUAL
endif

ifeq ($(SENSOR_TYPE0), SONY_IMX226_LVDS_12M_30FPS)
	SENSOR_PATH0 = $(PLATFORM)/sensor/sony_imx226
	SENSOR_LIB0 = $(PLATFORM)/sensor/sony_imx226/libsns_imx226.a
else ifeq ($(SENSOR_TYPE0), SONY_IMX274_MIPI_8M_30FPS)
	SENSOR_PATH0 = $(PLATFORM)/sensor/sony_imx274_mipi
	SENSOR_LIB0 = $(PLATFORM)/sensor/sony_imx274_mipi/libsns_imx274.a
else ifeq ($(SENSOR_TYPE0), SONY_IMX327_MIPI_1080P_30FPS)
	SENSOR_PATH0 := $(PLATFORM)/sensor/sony_imx327_mipi
	SENSOR_LIB0 = $(PLATFORM)/sensor/sony_imx327_mipi/libsns_imx327.a
endif

export SENSOR_PATH0
export SENSOR_LIB0

ifeq ($(SENSOR_TYPE1), SENSOR_VIRTUAL)
	SENSOR_PATH1 = $(PLATFORM)/sensor/virtual
	SENSOR_LIB1 = $(PLATFORM)/sensor/virtual/libsns_virtual.a
endif

export SENSOR_PATH1
export SENSOR_LIB1

LIBS=
LIBS+=$(PLATFORM)/libmpp.a
LIBS+=$(SENSOR_LIB0)
LIBS+=$(SENSOR_LIB1)
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
LIBS+=$(PLATFORM)/common_lib/libcurl.a
LIBS+=$(PLATFORM)/common_lib/libdigest.a
LIBS+=$(PLATFORM)/common_lib/libixml.a
LIBS+=$(PLATFORM)/common_lib/libmd5.a
LIBS+=$(PLATFORM)/common_lib/libtinyxml.a
LIBS+=$(PLATFORM)/common_lib/libturbojpeg.a
LIBS+=$(PLATFORM)/common_lib/libmp4v2.a
LIBS+=$(PLATFORM)/common_lib/libihwdet.a
export LIBS

CFLAGS += -g -Wall -fPIC -Os
CFLAGS += -mcpu=cortex-a17.cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations
CFLAGS += -DSENSOR_TYPE0=$(SENSOR_TYPE0)
CFLAGS += -DSENSOR_TYPE1=$(SENSOR_TYPE1)
CFLAGS += -D$(HIARCH)
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

