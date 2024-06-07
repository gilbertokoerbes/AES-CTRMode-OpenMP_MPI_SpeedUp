#!/bin/bash

x=1
while [ $x -le 48 ]
do
    echo "./aes_encrypt_ctr static "$x
    ./aes_encrypt_ctr static $x;
    ./aes_encrypt_ctr static $x;
    ./aes_encrypt_ctr static $x;

    echo "./aes_encrypt_ctr dynamic "$x
    ./aes_encrypt_ctr dynamic $x;
    ./aes_encrypt_ctr dynamic $x;
    ./aes_encrypt_ctr dynamic $x;

    echo "./aes_encrypt_ctr guided "$x
    ./aes_encrypt_ctr guided $x;
    ./aes_encrypt_ctr guided $x;
    ./aes_encrypt_ctr guided $x;

    echo "./aes_encrypt_ctr auto "$x
    ./aes_encrypt_ctr auto $x;
    ./aes_encrypt_ctr auto $x;
    ./aes_encrypt_ctr auto $x;

    x=$(( $x * 2 ))
done
