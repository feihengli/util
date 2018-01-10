#!/bin/sh
#编译所有第三方开源库，并拷贝静态库和头文件到当前目录
#注意解压源码后的目录不能与存放头文件的目录重名

#COMPILE=arm-hisiv300-linux-
COMPILE=arm-hisiv600-linux-

echo "COMPILE=${COMPILE}"
sleep 1

#libcurl.a
rm -rf libcurl
rm -rf curl-7.53.1
mkdir -p libcurl
tar -xzvf curl-7.53.1.tar.gz
cd ./curl-7.53.1;
./configure CC=${COMPILE}gcc --host=arm-linux
make
cp ./lib/.libs/libcurl.a ../ -af
cp include/curl/*.h ../libcurl -af
cd ../

#libixml.a
rm -rf libixml
rm -rf libixml-1.0
mkdir -p libixml
tar -xzvf libixml-1.0.tar.gz
cd ./libixml-1.0;
make CROSS=${COMPILE}
cp ./release/libixml.a ../ -af
cp ./*.h ../libixml -af
cd ../



