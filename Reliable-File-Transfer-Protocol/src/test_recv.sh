#!/bin/bash
if [ -z "${1+xxx}" ]
then
echo "need port as first argument"
exit 1
fi
if [ $1 -le 1024 ]
then
echo "port not valid"
exit 1
fi
 
if [ ! -x ./recvfile ]
then
echo "recvfile not found or executable"
exit 1
fi

echo "using port $1"
command="./recvfile -p $1"

# Create a symbolic link to ../testfiles
rm -f testfiles_symlink
ln -s ../testfiles/ testfiles_symlink

# all 0, 1k

if [ $2 -eq 1 ]
then
	filename=testfiles_symlink/1k.random
	timelimit=20
fi

# all 0, 50k

if [ $2 -eq 2 ]
then
    filename=testfiles_symlink/50k.random
    timelimit=20
fi

# all 0, 1m

if [ $2 -eq 3 ]
then
    filename=testfiles_symlink/1m.random
    timelimit=30
fi

# 20 each, 1m

if [ $2 -eq 4 ] || [ $2 -eq 5 ] || [ $2 -eq 6 ] || [ $2 -eq 7 ] || [ $2 -eq 8 ]
then
    filename=testfiles_symlink/1m.random
    timelimit=300
fi

# all 20, 1m

if [ $2 -eq 9 ]
then
    filename=testfiles_symlink/1m.random
    timelimit=600
fi

# alk 0, 1m

if [ $2 -eq 10 ]
then
    filename=testfiles_symlink/1m.random
    timelimit=60
fi

# all 2, 1m

if [ $2 -eq 11 ]
then
    filename=testfiles_symlink/1m.random
    timelimit=60
fi

# all 0, big

if [ $2 -eq 12 ]
then
    filename=testfiles_symlink/30m.bigfile
    timelimit=120
fi

# all 2, big

if [ $2 -eq 13 ]
then
    filename=testfiles_symlink/30m.bigfile
    timelimit=600
fi

echo "======START======"
echo "test "$2":"
echo "Using file: "$filename
echo ""

if [ $2 -eq 10 ] || [ $2 -eq 11 ]
then
        iteration=20
else
        iteration=1
fi

count=0

while [ $count -lt $iteration ]
do

echo ""
echo "round "$((count+1))
echo ""

if [ -e $filename.recv ]
then
	rm $filename.recv
fi

$command > /dev/null &
pid=$!

time=0
while [ ! -e $filename.recv ] 
do
	sleep 1
	echo -n "* "
	time=$((time+1))
	if [ $time -gt $timelimit ]
	then
		if [ -z "pgrep recvfile" ]
		then
			echo ""
			echo "Error: receiving process crashed"
			break
		else
			echo ""
			echo "Error: cannot receive file name"
			kill $pid
			break
		fi
	fi
done

if [ -e $filename.recv ]
then
	time=0
	# while [ ! $(stat -c%s $filename) = $(stat -c%s $filename.recv) ]
	# while [ 1 = $(($(date +%s)-$(stat -c%Y $filename.recv)<1)) ]
	while [ ! $(stat -c%s $filename) = $(stat -c%s $filename.recv) ] || [ 1 = $(($(date +%s)-$(stat -c%Y $filename.recv)<1)) ]
	do
        	sleep 1
        	echo -n "."
       		time=$((time+1))
        	if [ $time -gt $timelimit ]
             	then
	   		echo ""
			echo "Error: transmission cannot finish"
                	kill $pid
			break
        	fi
	done
	
	if [ $(stat -c%s $filename) = $(stat -c%s $filename.recv) ]
	then
		echo ""
		echo "checksum check..."
		md5sum -c $filename.md5sum
	fi
fi

if [ ! -z "pgrep recvfile" ]
then
	echo ""
	echo "Error: receiver is not closed"
	kill $pid
fi

count=$((count+1))

done

