#!/bin/sh
#编译所有第三方开源库，并拷贝静态库到当前目录

#libcurl.a
rm -rf curl-7.53.1
tar -xzvf curl-7.53.1.tar.gz
cd ./curl-7.53.1;
./configure
make
cp ./lib/.libs/libcurl.a ../ -af
cd ../

#libixml.a
rm -rf libixml-1.0
tar -xzvf libixml-1.0.tar.gz
cd ./libixml-1.0;
make CROSS=
cp ./release/libixml.a ../ -af
cd ../

#libjpeg.a
rm -rf jpeg-8c
tar -xzvf jpegsrc.v8c.tar.gz
cd ./jpeg-8c;
./configure
make
cp ./.libs/libjpeg.a ../ -af
cd ../

#libturbojpeg.a
rm -rf libjpeg-turbo-1.5.3
tar -xzvf libjpeg-turbo-1.5.3.tar.gz
cd ./libjpeg-turbo-1.5.3;
./configure
make
cp ./.libs/libturbojpeg.a ../ -af
cd ../

#libmd5.a
rm -rf libmd5
tar -xzvf libmd5.tar.gz
cd ./libmd5;
make CROSS_COMPILE=
cp ./libmd5.a ../ -af
cd ../

#libtinyxml.a
rm -rf libtinyxml
tar -xzvf libtinyxml.tar.gz
cd ./libtinyxml;
make CROSS_COMPILE=
cp ./libtinyxml.a ../ -af
cd ../

#libdigest.a
rm -rf libdigest
tar -xzvf libdigest.tar.gz
cd ./libdigest;
make CROSS_COMPILE=
cp ./libdigest.a ../ -af
cd ../

#libmp4v2.a
rm -rf mp4v2-2.0.0
tar -xjvf mp4v2-2.0.0.tar.bz2
cd ./mp4v2-2.0.0;
./configure
make
strip --strip-debug --strip-unneeded ./.libs/libmp4v2.a
ranlib ./.libs/libmp4v2.a
cp ./.libs/libmp4v2.a ../ -af
cd ../

#iperf3
rm -rf iperf3 libiperf.so.0
rm -rf iperf-3.1.3
tar -xzvf iperf-3.1.3-source.tar.gz
cd ./iperf-3.1.3;
./configure
make
cp ./src/.libs/iperf3 ../ -af
cp ./src/.libs/libiperf.so.0.0.0 ../libiperf.so.0 -af
cd ../


#strip --strip-debug --strip-unneeded *.a