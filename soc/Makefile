
include ../Rules.make

ifeq ($(HIARCH), hi3518ev200)
	PLATFORM_PATH := ./hi3518ev200
endif


all:
	make -C $(PLATFORM_PATH)
	cp -af $(PLATFORM_PATH)/*.a ./
	cp -af $(PLATFORM_PATH)/mpp_lib/*.a ./

clean:
	make -C $(PLATFORM_PATH) clean
	rm -rf ./*.a