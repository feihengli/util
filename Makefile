# Hisilicon Hi3516 sample Makefile

include Rules.make

PWD := $(shell pwd)
ROOTDIR := $(PWD)/../../
SENSOR_LIB_PATH := $(PWD)/sensor/lib

ifeq ($(SENSOR_TYPE), APTINA_AR0130_DC_720P_30FPS)
	SENSOR_PATH := $(SENSOR_LIB_PATH)/../ar0130/
	SENSOR_LIBS := $(SENSOR_LIB_PATH)/libsns_ar0130.a
endif

ifeq ($(SENSOR_TYPE), OMNIVISION_OV9732_MIPI_720P_30FPS)
	SENSOR_PATH := $(SENSOR_LIB_PATH)/../omnivision_ov9732_mipi/
	SENSOR_LIBS := $(SENSOR_LIB_PATH)/libsns_ov9732.a
endif

ifeq ($(SENSOR_TYPE), OMNIVISION_OV2710_MIPI_1080P_30FPS)
	SENSOR_PATH := $(SENSOR_LIB_PATH)/../omnivision_ov2710_mipi/
	SENSOR_LIBS := $(SENSOR_LIB_PATH)/libsns_ov2710.a
endif

ifeq ($(SENSOR_TYPE), SONY_IMX291_LVDS_1080P_30FPS)
	SENSOR_PATH := $(SENSOR_LIB_PATH)/../sony_imx291/
	SENSOR_LIBS := $(SENSOR_LIB_PATH)/libsns_imx291.a
endif

ifeq ($(SENSOR_TYPE), SONY_IMX323_DC_1080P_30FPS)
	SENSOR_PATH := $(SENSOR_LIB_PATH)/../sony_imx323_i2c/
	SENSOR_LIBS := $(SENSOR_LIB_PATH)/libsns_imx323.a
endif

ifeq ($(SENSOR_TYPE), SMARTSENS_SC1135_DC_960P_30FPS)
	SENSOR_PATH := $(SENSOR_LIB_PATH)/../sc1135/
	SENSOR_LIBS := $(SENSOR_LIB_PATH)/libsns_sc1135.a
endif

ifeq ($(SENSOR_TYPE), APTINA_AR0237_DC_1080P_30FPS)
	SENSOR_PATH := $(SENSOR_LIB_PATH)/../aptina_ar0237/
	SENSOR_LIBS := $(SENSOR_LIB_PATH)/libsns_ar0237.a
endif

ifeq ($(SENSOR_TYPE), SMARTSENS_SC2135_DC_1080P_30FPS)
	SENSOR_PATH := $(SENSOR_LIB_PATH)/../sc2135/
	SENSOR_LIBS := $(SENSOR_LIB_PATH)/libsns_sc2135.a
endif

CFLAGS := -Wall -g
CFLAGS += -DSENSOR_TYPE=$(SENSOR_TYPE)
CFLAGS += -I$(MPP_PATH)/include/


MPI_LIBS := $(SDK_PATH)/mpp/lib/libmpi.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/libive.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/libmd.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/libVoiceEngine.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/libupvqe.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/libdnvqe.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/libisp.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/lib_cmoscfg.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/lib_hiae.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/lib_hiawb.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/lib_hidefog.a
MPI_LIBS += $(SDK_PATH)/mpp/lib/lib_hiaf.a
MPI_LIBS += $(SENSOR_LIBS)
MPI_LIBS += $(PWD)/libixml/libixml.a
MPI_LIBS += $(PWD)/libtinyxml/libtinyxml.a
MPI_LIBS += $(PWD)/digest/digest.a
MPI_LIBS += $(PWD)/libcurl/libcurl.a
MPI_LIBS += $(PWD)/md5/md5.a

SRC=$(wildcard *.c)

OBJS=$(patsubst %.c,%.o,$(SRC))

TARGET=libipc.so


$(TARGET): $(OBJS)
	make -C $(SENSOR_PATH)
	@echo
	$(LINK) -shared -fPIC -lpthread -lm -ldl -Wl,-gc-sections -o $@ $^ $(MPI_LIBS)
	$(STRIP) $@
	cp $@ /mnt/hgfs/nfs/bpc/ -af

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo

clean:
	make -C $(SENSOR_PATH) clean
	@echo
	rm -f $(TARGET)
	rm -f *.o $(OBJ_PATH)/*.o

sdk_release:
	@rm -rf ./release/*.h
	@rm -rf release.tar.gz
	cp $(TARGET) ./release -af
	cp sal_standard.h ./release -af
	cp sal_av.h ./release -af
	cp sal_audio.h ./release -af
	cp sal_upgrade.h ./release -af
	cp sal_ircut.h ./release -af
	cp sal_md.h ./release -af
	#cp sal_isp.h ./release -af
	#cp sal_ircut.h ./release -af
	#cp sal_lbr.h ./release -af
	#cp sal_osd.h ./release -af
	#cp sal_yuv.h ./release -af
	tar -czvf release.tar.gz release

binary:
	@mkdir -p ./bin
	cp $(TARGET) ./bin -af
	@echo
	make -C ./driver clean
	make -C ./driver
	cp ./driver/board_ctl.ko ./bin -af
	@echo
	make -C ./debugc clean
	make -C ./debugc
	cp ./debugc/debugc ./bin -af
	@echo
	make -C ./ledCtrl clean
	make -C ./ledCtrl
	cp ./ledCtrl/ledCtrl ./bin -af
	@echo
	make -C ./ntpc clean
	make -C ./ntpc
	cp ./ntpc/ntpc ./bin -af
	@echo
	make -C ./qr_scan clean
	make -C ./qr_scan
	cp ./qr_scan/qr_scan ./bin -af
	@echo
	make -C ./sys_daemon clean
	make -C ./sys_daemon
	cp ./sys_daemon/sys_daemon ./bin -af
	@echo
	make -C ./test_encode clean
	make -C ./test_encode
	cp ./test_encode/test_encode ./bin -af
	@echo
	cp ./dzk/gbk.dzk ./bin -af
	cp ./dzk/asc16.dzk ./bin -af
	@echo
	cp ./config/ov2710.xml ./bin -af
	cp ./config/title.cfg ./bin -af
	cp ./config/mktech.sh ./bin -af
	cp ./config/init.sh ./bin -af
	@echo
	make -C ./rtsp_server clean
	make -C ./rtsp_server
	cp ./rtsp_server/rtsp_server ./bin -af
	@echo
	make -C ./broadcast clean
	make -C ./broadcast
	cp ./broadcast/broadcast ./bin -af
	@echo
	make -C ./atest clean
	make -C ./atest
	cp ./atest/atest ./bin -af
