#! /bin/sh

ROOT=$PWD
CMT=$ROOT/comments
PUBCMT=pub.cmt
LOG=uncmt.log

subs_files()
{
	count=`expr ${count} + 1`
	echo "    " $count: subs file .${PWD##$ROOT}/$1

	FILE=$1
	cat $FILE > $FILE.tmp

	cmt_files=`grep "^// .*.cmt" $FILE | awk '{ print $2 ;}'`
	echo "\t\t" find [$cmt_files] in $1
	if [ "$cmt_files" = "" ]
	then
		echo $count: no cmt file .${PWD##$ROOT}/$1 >> $ROOT/$LOG
	#	ls -l $1 >> $ROOT/$LOG
	fi
	touch $FILE.tmp

	for cmt_file in $cmt_files
	do
		#echo -n "\t\t" -\> substitute $cmt_file ...
		cat $CMT/$PUBCMT $CMT/$cmt_file > cmt.tmp
		sed ':label;N;s/ \*\/\n\// /;b label' cmt.tmp > cmt.tmp2
		sed '/'$cmt_file'/r cmt.tmp2' $FILE > $FILE.tmp
		sed '/'$cmt_file'/d' $FILE.tmp > $FILE
		rm cmt.tmp
		rm cmt.tmp2
		#echo " ok!" 
	done

	rm $FILE.tmp
}

subs_dir()
{
	all=`ls .`

	for i in $all
	do
		if [ -f "$i" ]
		then 
			# echo "   " \* checking file $i
			cfile=`echo $i | awk -F. '{print $NF}'`
			# echo $cfile
			if [ "$cfile" = "c" -o "$cfile" = "h" ]
			then 
				subs_files $i
			fi 
		fi

		if [ -d "$i" ]
		then
			cd $i
			#echo + entering $i
			subs_dir 
			cd ..
		fi
	done
}

if [ "$1" = "commit" ]
then
	git add .
	git commit -a -m "add cmt files"
	git push
	exit
fi

cd $CMT
cmt_files=`ls *.cmt`
#echo $cmt_files
cd ..

SRC=$1
DST=$1-subs
echo $DST

if [ "$1" = "" ]
then
	echo Your must enter the dir name 
	echo For example: ./s.sh testdir
	exit
fi

rm -rf $DST
mkdir $DST
cp $SRC/* $DST -r

rm $LOG
touch $LOG
cd $DST
DIR=$DIR/$1
subs_dir 
cd ..

echo -----
cat $LOG

echo Total `wc -l $LOG` files need to comment
echo Total `find $1 -name "*.c" | wc -l` \*.c files
diff -Nur $SRC $DST > diff.$SRC
ls -l diff.$SRC
echo `du -bs comments` bytes
echo Total cmt fils `ls -l comments/*.cmt | wc -l`

exit
