#!/bin/sh
echo " kill old process...."


killall telnetd
killall demo
killall qr_scan
killall wpa_supplicant
killall udhcpc
killall ledCtrl
ledCtrl -r &

ifconfig wlan0 down
ifconfig wlan0 up

sequenceid_path=/tmp/sequenceid
product_id=ipcsn555
if [ -s /etc/product_id ]; then
	product_id=$(cat /etc/product_id)
	echo "product_id reset: $product_id"
fi

qr_scan_enable=1
if [ -s /etc/wpa_supplicant.conf ]; then
	qr_scan_enable=0
fi

if [ $qr_scan_enable == 1 ];then
	echo " start qrscan...."
	echo "red light flicker ..."
	killall ledCtrl
	ledCtrl -rf &
	echo
	sleep 1
	
	rm /tmp/wpa_supplicant.conf -rf
	rm $sequenceid_path -rf
	qr_scan /tmp/wpa_supplicant.conf $sequenceid_path &
	sleep 1

	while [ 1 ]                                                  
	do                                                           
		echo "waiting qrscan done..."                           
		sleep 1                                                  

		if [ -s /tmp/wpa_supplicant.conf ]; then
			echo "wait qr_scan to write done."
			sleep 1
			killall qr_scan
			cp -af /tmp/wpa_supplicant.conf /etc/wpa_supplicant.conf
			break                         
		fi                               
	done
fi


echo " start WiFi...."
echo "green light flicker ..."
killall ledCtrl
ledCtrl -gf &
echo
sleep 1

wpa_supplicant -Dwext -iwlan0 -B -c/etc/wpa_supplicant.conf

tm=0
rm /tmp/debug_wlan_connected -rf
while [ 1 ]
do
    echo "waiting wlan connect... $tm second"
	if [ $tm -ge 30 ]; then
		echo "time out $tm..."
		break
    fi
    sleep 1
    #iwconfig wlan0 | grep 'ESSID' > /tmp/debug_wlan_connected
	wpa_cli stat | grep 'wpa_state=COMPLETED' > /tmp/debug_wlan_connected
    if [ -s /tmp/debug_wlan_connected ]; then
       break
    fi
	let "tm = $tm + 1"
done

sleep 1
udhcpc -i wlan0 -s /etc/udhcpc.script

#echo "starting Ftp server...."
echo "already connected.turn on green light."
killall ledCtrl
ledCtrl -g &
echo

sleep 1

killall broadcast
if [ -s $sequenceid_path ]; then
	sequenceid=$(cat $sequenceid_path)
	echo "broadcast sequenceid: $sequenceid"
	broadcast $sequenceid $product_id &
	break
fi

t=0
rm /tmp/debug_ntpc_sync -rf
while [ 1 ]
do
    echo "waiting ntpc sync time $t second..."
	killall ntpc
	ntpc | grep 'response' > /tmp/debug_ntpc_sync &
	if [ $t -ge 5 ]; then
		echo "time out $t..."
		break
    fi
    sleep 1
    if [ -s /tmp/debug_ntpc_sync ]; then
		echo "ntpc sync time done $t second..."
		cat /tmp/debug_ntpc_sync
		break
    fi
	let "t = $t + 1"
done
	
cd /usr/bin;
./demo -w 1920 -h 1080 -f 15 -s $product_id -p 123456 -d &
telnetd &





