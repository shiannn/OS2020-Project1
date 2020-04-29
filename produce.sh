#! /bin/bash

DATA_FOLDER="./OS_PJ1_Test"
mkdir output -p
for data in $DATA_FOLDER/*;
do
    echo $data;
    tmp=${data%.txt}
    fname=${tmp##*/}
    outfile=./output/${fname}_stdout.txt
    msgfile=./output/${fname}_dmesg.txt
    sudo dmesg -c
    sudo ./a.out < $data > $outfile
    sudo dmesg | grep Project1 > $msgfile
done

echo "finish"