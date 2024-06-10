#!/bin/bash

N=$1 ##numero nodos do cluster
echo $N

np=8 #numero de processos criados no S.O
echo 'np ' $np
max_np=$((16*$N))
echo 'max_np ' $max_np
while [ $np -le $max_np ]
do
echo "np "$np

    b=1 #numero de blocos por mensagem
    while [ $b -le 4 ] 
    do
    echo "b "$b

        reply=0
        while [ $reply -le 2 ] 
        do 

        sed -i "s/\(#define MESSAGE_BLOCKS \)[0-9]\+/\1$b/" aes_encrypt_ctr_mpi_openmp.c
        ladcomp -env  mpiompcc  aes_encrypt_ctr_mpi.c /home/cp03/tiny-AES-c/aes.c -o aes_encrypt_ctr -I/home/cp03/tiny-AES-c
        ladrun -np  $np  ./aes_encrypt_ctr $N

        ((reply++))
        done

    b=$((b*2))
    done

np=$((np*2))
done

