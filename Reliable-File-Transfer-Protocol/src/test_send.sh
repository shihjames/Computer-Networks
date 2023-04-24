#!/bin/bash

HOST=128.42.124.176

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
 
if [ ! -x ./sendfile ]
then
	echo "sendfile not found or executable"
	exit 1
fi

echo "using port $1"

# Create a symbolic link to ../testfiles
rm -f testfiles_symlink
ln -s ../testfiles/ testfiles_symlink

# all 0, 1k 
if [ $2 -eq 1 ]
then
	file=testfiles_symlink/1k.random
	testparam=" "
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

	# test
	netsim $testparam &>/dev/null
	echo $command
	$command 2>&1 > /dev/null
fi

# all 0, 50k
if [ $2 -eq 2 ]
then
	file=testfiles_symlink/50k.random
	testparam=" "
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

        # test
        netsim $testparam &>/dev/null
        echo $command
        $command 2>&1 > /dev/null
fi

# all 0, 1m
if [ $2 -eq 3 ]
then
	file=testfiles_symlink/1m.random
	testparam=" "
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

        # test
        netsim $testparam &>/dev/null
        echo $command
        $command 2>&1 > /dev/null
fi

# 20 each, 1m
if [ $2 -eq 4 ]
then
	file=testfiles_symlink/1m.random
	testparam="--delay 20"
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

        # test
        netsim $testparam &>/dev/null
        echo $command
        $command 2>&1 > /dev/null
fi

if [ $2 -eq 5 ]
then
	file=testfiles_symlink/1m.random
	testparam="--drop 20"
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

        # test
        netsim $testparam &>/dev/null
        echo $command
        $command 2>&1 > /dev/null
fi

if [ $2 -eq 6 ]
then
	file=testfiles_symlink/1m.random
	testparam="--reorder 20"
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

        # test
        netsim $testparam &>/dev/null
        echo $command
        $command 2>&1 > /dev/null
fi

if [ $2 -eq 7 ]
then
	file=testfiles_symlink/1m.random
	testparam="--mangle 20"
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

        # test
        netsim $testparam &>/dev/null
        echo $command
        $command 2>&1 > /dev/null
fi

if [ $2 -eq 8 ]
then
	file=testfiles_symlink/1m.random
	testparam="--duplicate 20"
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

        # test
        netsim $testparam &>/dev/null
        echo $command
        $command 2>&1 > /dev/null
fi

# all 20, 1m
if [ $2 -eq 9 ]
then
	file=testfiles_symlink/1m.random
	testparam="--delay 20 --drop 20 --reorder 20 --mangle 20 --duplicate 20"
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

        # test
        netsim $testparam &>/dev/null
        echo $command
        $command 2>&1 > /dev/null
fi

# all 2, 1m (time)
if [ $2 -eq 10 ]
then
        file=testfiles_symlink/1m.random
        testparam=" "
        echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
        command="./sendfile -r $HOST:$1 -f $file"

	count=0
	while [ $count -lt 20 ]
	do
		# test
		netsim $testparam &>/dev/null
		#echo $command

		T="$(date +%s%N)"
		$command 2>&1 > /dev/null
		T="$(($(date +%s%N)-T))"
		echo "${T}" >> tmp
		sleep 2
		count=$((count+1))
	done

	sort -n tmp -o tmp
	declare -a sortarray
	
	while read line 
	do
        	sortarray=(${sortarray[@]} $line)
	done < tmp
	rm tmp

	i=4
	total=0
	while [ $i -lt 12 ]
	do
		total=$(( $total + ${sortarray[$i]} ))	
		i=$((i+1))
	done

	total=$((total/12))
	echo "Average transmission time is "$total
fi


# all 2, 1m (time)
if [ $2 -eq 11 ]
then
	file=testfiles_symlink/1m.random
	testparam="--delay 2 --drop 2 --reorder 2 --mangle 2 --duplicate 2"
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

        count=0
        while [ $count -lt 20 ]
        do
                # test
                netsim $testparam &>/dev/null
                #echo $command

                T="$(date +%s%N)"
                $command 2>&1 > /dev/null
                T="$(($(date +%s%N)-T))"
                echo "${T}" >> tmp
                sleep 2
                count=$((count+1))
        done

        sort -n tmp -o tmp
        declare -a sortarray

        while read line 
        do
                sortarray=(${sortarray[@]} $line)
        done < tmp
        rm tmp

        i=4
        total=0
        while [ $i -lt 12 ]
        do
                total=$(( $total + ${sortarray[$i]} ))
                i=$((i+1))
        done

        total=$((total/12))
        echo "Average transmission time is "$total
fi

# all 0, big
if [ $2 -eq 12 ]
then
	file=testfiles_symlink/30m.bigfile
	testparam=" "
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

        # test
        netsim $testparam &>/dev/null
        echo $command
        $command 2>&1 > /dev/null
fi

# all 2, big
if [ $2 -eq 13 ]
then
	file=testfiles_symlink/30m.bigfile
	testparam="--delay 2 --drop 2 --reorder 2 --mangle 2 --duplicate 2"
	echo "@@@@ test #$2 [$testparam] | FILE $file @@@@"
	command="./sendfile -r $HOST:$1 -f $file"

	# test
        netsim $testparam &>/dev/null
        echo $command
        $command 2>&1 > /dev/null
fi

echo "======finish======"