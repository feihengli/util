/************************************************************************************************************/
You should change HZ config in kernel when sensor is running at 120fps or above. 
Because default HZ is 100, meaning 10ms tick;You can change HZ to 200,300,or other.
/************************************************************************************************************/

/************************************************************************************************************/
If use VI-VPSS online mode,please make sure that the ISP and VPSS clocks are the same.
/************************************************************************************************************/

The following sensor libraries are currently only available on the HI3519V101:

/************************************************************************************************************/
IMX226 :
 4000*3000@30fps or 3840*2160@60fps£¬you must set the VI clock to 500MHz or more.
	Replace "himm 0x12010054 0x00004043" with "himm 0x12010054 0x00024043" in mpp_xxx/ko/clkcfg_hi3519v101.sh.
	When uesd 4K*2k 2to1_frame WDR, frame rate of VI is up to 60£¬you must set the VI and isp clock to 600MHz.
	When uesd 4K*3k 2to1_frame WDR,frame rate of VI is up to 30£¬you must set the VI and isp clock to 500MHz or more¡£
	When uesd 4K*3K@30£¬you must set the VI and ISP clock to 500MHz or more¡£
 	Modefy mpp_xxx/ko/clkcfg_hi3519v101.sh "himm 0x1201004c 0x00094821,himm 0x12010054 0x4041" to corresponding value¡£
/************************************************************************************************************/

/************************************************************************************************************/
IMX274 :
	When uesd 4K*2k 2to1_frame WDR,frame rate of VI is up to 60£¬you must set the VI clock to 600MHz , set isp clock to 300M or more.
	Modefy mpp_xxx/ko/clkcfg_hi3519v101.sh "himm 0x1201004c 0x00094821,himm 0x12010054 0x4041" to corresponding value¡£
/************************************************************************************************************/

/************************************************************************************************************/
IMX290 :
	When uesd 1080P@30 2to1_line WDR,frame rate of VI is up to 60£¬you must set the VI clock to 300MHz or more,set isp clock to 214MHz or more.
	When uesd 1080P@30 3to1_line WDR,frame rate of VI is up to 90£¬you must set the VI clock to 340MHz or more,set isp clock to 214MHz or more.
	Modefy mpp_xxx/ko/clkcfg_hi3519v101.sh "himm 0x1201004c 0x00094821" to corresponding value¡£
/************************************************************************************************************/

/************************************************************************************************************/
OS05A :
	When uesd 5M@30 2to1_line WDR,frame rate of VI is up to 30£¬you must set the VI clock to 600MHz or more,set isp clock to 214MHz or more.
	Modefy mpp_xxx/ko/clkcfg_hi3519v101.sh "himm 0x1201004c 0x00094821" to corresponding value¡£
/************************************************************************************************************/

The following sensor libraries are currently only available on the HI3559V100:

/************************************************************************************************************/
IMX377 :
	When uesd 1080P@120fps¡¢720P@240fps¡¢4k*2K@60»ò12M@30£¬you must set the VI and isp clock to 600MHz.
	Modefy mpp_xxx/ko/clkcfg_hi3519v101.sh "himm 0x1201004c 0x00094821,himm 0x12010054 0x4041" to corresponding value¡£
/************************************************************************************************************/

/************************************************************************************************************/
mn34120 :
  	When uesd 8M@30£¬you must set the VI and isp clock to 300MHz or more.
	When uesd 16M@16.25£¬you must set the VI clock to 300MHz or more.
	Modefy mpp_xxx/ko/clkcfg_hi3519v101.sh "himm 0x1201004c 0x00094821,himm 0x12010054 0x4041" to corresponding value¡£
/************************************************************************************************************/

/************************************************************************************************************/
imx117:
  	When uesd 8M@30£¬you must set the VI and isp clock to 300MHz.
	When uesd 8M@60£¬you must set the VI and isp clock to 600MHz.
	When uesd 12M@28£¬you must set the VI clock to 396MHz or more.
	Modefy mpp_xxx/ko/clkcfg_hi3519v101.sh "himm 0x1201004c 0x00094821,himm 0x12010054 0x4041" to corresponding value¡£
/************************************************************************************************************/