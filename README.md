### Comandos uteis

Comandos uteis para serem utilizados dentro do Cluster

- Alocar um Cluster <br>
```ladalloc -n 1 -t 10 -s/e```

- Compilar e executar <br>
``` gcc -fopenmp aes_encrypt_ctr.c /home/cp03/tiny-AES-c/aes.c -o aes_encrypt_ctr -I/home/cp03/tiny-AES-c && echo '' && ./aes_encrypt_ctr <mode> <threads> ```

- Gerar um arquivo de tamanho (bs * count) na memoria: <br>
```  dd if=/dev/zero bs=1M count=248 | tr '\000' '\001' > /dev/shm/large_file.dat ```

- Verificar o arquivo: <br>
``` hexdump -C /dev/shm/large_file.dat ```

- Compilar no MPi e executar
```ladcomp -env mpicc aes_encrypt_ctr_mpi_openmp.c /home/cp03/tiny-AES-c/aes.c -o aes_encrypt_ctr -I/home/cp03/tiny-AES-c ; ladrun -np 2 ./aes_encrypt_ctr```

- Compilar no MPi com OpemMP e executar
```ladcomp -env  mpiompcc  aes_encrypt_ctr_mpi_openmp.c /home/cp03/tiny-AES-c/aes.c -o aes_encrypt_ctr -I/home/cp03/tiny-AES-c```
Executar

 ```ladrun -np 2 ./aes_encrypt_ctr 2```