#! /bin/sh


#自动根据节点文件名来更新md5值和文件长度两个节点
#

ORG=upgrade_cfg.xml
TMP=upgrade_cfg.xml.tmp
TMP2=upgrade_cfg.xml.tmp2
rm -f $TMP $TMP2

#file find and replace
func()
{
	sed -n "/"$1"/,/"$1"/p" $ORG > $TMP2
	filename=$(grep name $TMP2 | sed -nr '/<name>.*<\/name>/s_.*<name>([^<]*)</name>.*_\1_p')
	md5value=$(md5sum $filename | head -c 32)
	filesize=$(stat -c%s $filename)
	echo "$1, $filename, $md5value, $filesize"
	sed "s/<md5>.*<\/md5>/<md5>$md5value<\/md5>/g" -i $TMP2
	sed "s/<length>.*<\/length>/<length>$filesize<\/length>/g" -i $TMP2
	cat $TMP2 >> $TMP
}

echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" >> $TMP
echo "<root>" >> $TMP
grep version_info $ORG >> $TMP

func file0
func file1
func file2
func file3
func file4
func file5
func file6

echo "</root>" >> $TMP

cp $TMP $ORG -af
rm -f $TMP $TMP2







