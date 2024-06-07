#!/bin/bash
N=$(ladqueue | awk '{print $7}' | tail -n 1) ##numero nodos do cluster
echo $N
np=2 #numero de processos criados no S.O
max_np=$((16*$N))
echo 'max_np ' $max_np
b=1 #numero de blocos por mensagem



while [ $np -le $max_np ]
do
echo "np "$np

while [ $b -le 90 ] 
do
echo "b "$b

reply=0
while [ $reply -le 3 ] 
do 

sed -i "s/\(#define MESSAGE_BLOCKS \)[0-9]\+/\1$b/" aes_encrypt_ctr_mpi_openmp.c
ladcomp -env  mpiompcc  aes_encrypt_ctr_mpi_openmp.c /home/cp03/tiny-AES-c/aes.c -o aes_encrypt_ctr -I/home/cp03/tiny-AES-c
ladrun -np  $np  ./aes_encrypt_ctr $N

((reply++))
done

b=$((b*4))
done

np=$((n*2))
done

