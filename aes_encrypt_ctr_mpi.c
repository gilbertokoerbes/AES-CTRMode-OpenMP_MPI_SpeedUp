//  gcc -fopenmp aes_encrypt_ctr.c /home/cp03/tiny-AES-c/aes.c -o aes_encrypt_ctr -I/home/cp03/tiny-AES-c && echo '' &&time ./aes_encrypt_ctr
//  dd if=/dev/zero bs=1M count=512 | tr '\000' '\001' > /dev/shm/large_file.dat
// hexdump -C /dev/shm/large_file.dat
// ladalloc -n 1 -t 10 -s/e

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mpi.h"

#include <time.h>


#define AES128 1
#define AES_BLOCK_SIZE 16
#define ECB 1
#define CBC 1
#define CTR 1
#define DEBUG 0


#include "aes.h"

typedef struct {
    int i;
    char key[16]
    char nonce[16]
    char *message; // Ponteiro para a string
} CustomMessage;

// Função para gerar o próximo bloco de CTR
void generate_ctr_block(unsigned char *nonce, unsigned int counter, unsigned char *output) {
    // Copia o nonce para o bloco de saída
    memcpy(output, nonce, AES_BLOCK_SIZE);

    // Converte o contador para bytes e copia para o final do bloco de saída
    for (int i = AES_BLOCK_SIZE - sizeof(unsigned int); i < AES_BLOCK_SIZE; ++i) {
        output[i] = (counter >> ((AES_BLOCK_SIZE - i - 1) * 8)) & 0xFF;
    }
}


int main(int argc, char* argv[]){
    int my_rank, proc_n;
    MPI_Request request;
    MPI_Status status;
    CustomMessage msg;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <quantidade_blocos_por_slave>\n", argv[0]);
        return 1;
    }
    int message_blocks_slave = atoi(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n);
    // Alocação dinâmica para a mensagem
    msg.message = (char *)malloc(message_size * sizeof(char));
    if (msg.message == NULL) {
        fprintf(stderr, "Erro ao alocar memória\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // Definindo o tipo MPI para CustomMessage
    const int nitems = 4;
    int blocklengths[4] = {1, 16, 16, message_size};
    MPI_Datatype types[4] = {MPI_INT, MPI_CHAR, MPI_CHAR, MPI_CHAR};
    MPI_Datatype mpi_custom_message_type;
    MPI_Aint offsets[4];

    offsets[0] = offsetof(CustomMessage, i);
    offsets[1] = offsetof(CustomMessage, key);
    offsets[2] = offsetof(CustomMessage, nonce);
    offsets[3] = offsetof(CustomMessage, message);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_custom_message_type);
    MPI_Type_commit(&mpi_custom_message_type);

    if (my_rank == 0) {
        // Papel do mestre
        int num_workers = proc_n - 1;
    FILE *arquivo;
    FILE *fptr_output;
    FILE *fptr_executions;

    char buffer_in[AES_BLOCK_SIZE]; //blocos lidos conforme tamanho do bloco AES
    size_t blocos_lidos;
    long tamanho_arquivo;
    int num_blocks;

    fptr_output = fopen("/dev/shm/output_encrypt.txt", "w");
    fptr_executions = fopen("./output_results.txt", "a");

    // Abre o arquivo para leitura
    arquivo = fopen("/dev/shm/large_file.dat", "rb");
    if (arquivo == NULL) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }
    // Obtém o tamanho do arquivo
    fseek(arquivo, 0, SEEK_END); // Vai para o final do arquivo
    tamanho_arquivo = ftell(arquivo); // Obtém a posição atual (tamanho do arquivo)
    fseek(arquivo, 0, SEEK_SET); // Volta para o início do arquivo

    // Calcula o número total de blocos
    num_blocks = (int)(tamanho_arquivo / (AES_BLOCK_SIZE-1));
    
    if(DEBUG)
        printf("num_blocks %d", num_blocks);

    unsigned char *buffer_encrypted_output = (unsigned char *)malloc(AES_BLOCK_SIZE * num_blocks);
    if (!buffer_encrypted_output) {
        printf("Erro ao alocar memória.\n");
        return 1;
    }

//     // Chave AES de 128 bits (16 bytes)
    uint8_t key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    uint8_t nonce[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};  

    if(DEBUG){
        int key_length = sizeof(key);    
        printf("\n key  em hex\n");
        for(int i = 0; i < key_length; i++)
            printf("%02hhX", key[i]);
    }
    
    if(DEBUG){
        int nonce_length = sizeof(nonce);   
        printf("\n Nonce  em hex\n");
        for(int i = 0; i < nonce_length; i++)
            printf("%02hhX", nonce[i]);
        printf("\n");
    }
    // Criandobuffer_in para armazenar a saída
    //unsigned char *buffer_encrypted_output = (unsigned char *)malloc(num_blocks * AES_BLOCK_SIZE);

    struct timespec start, end;
    double time_spent;
    clock_gettime(CLOCK_MONOTONIC, &start);


    for (int i = 0; i < num_blocks; ++i) {
        
        fread(buffer_in, 1,AES_BLOCK_SIZE, arquivo);
        if(DEBUG)
        {
            int input_length = sizeof(buffer_in);    
            printf("\n in  em hex\n");
            for(int i = 0; i < input_length; i++)
                printf("%02hhX", buffer_in[i]);
        }

    }

//#################################################
// Slave
//#################################################

        unsigned char counter_block[AES_BLOCK_SIZE];
        //generate_ctr_block(nonce, i, counter_block);
        generate_ctr_block(nonce, i, counter_block);
        
        if(DEBUG)
        {
            int counter_block_length = sizeof(counter_block);    
            printf("\n counter_block  em hex\n");
            for(int i = 0; i < counter_block_length; i++)
            printf("%02X", counter_block[i]);
            printf("\n");
        }

    
        // Criptografa o bloco de contador usando AES ECB
        //uint8_t encrypted_counter_block[] = nonce;

        struct AES_ctx ctx;

        AES_init_ctx(&ctx, key);
        AES_ECB_encrypt(&ctx, counter_block);
        
        if(DEBUG)
        {
            int counter_block_length = sizeof(counter_block);    
            printf("\n counter_block  cifrado\n");
            for(int i = 0; i < counter_block_length; i++)
            printf("%02X", counter_block[i]);
            printf("\n");
        }
        // XOR entre o bloco criptografado do contador e o bloco de entrada
        for (int j = 0; j < AES_BLOCK_SIZE; ++j) {
            //buffer_encrypted_output[i * AES_BLOCK_SIZE + j] = encrypted_counter_block[j] ^ in[i * AES_BLOCK_SIZE + j];
            buffer_encrypted_output[i * AES_BLOCK_SIZE + j] = counter_block[j] ^buffer_in[j];
        }
        //fprintf(fptr_output, buffer_encrypted_output);
        if(DEBUG)
        {        
            printf("\n block cifrado\n");
            for(int i = 0; i < AES_BLOCK_SIZE; i++)
                printf("%02X", buffer_encrypted_output[i]);
                printf("\n");
        }
        //printf(".");
        

    }

    // Obter o tempo final
    clock_gettime(CLOCK_MONOTONIC, &end);
    // Calcular o tempo decorrido
    time_spent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    //for(int i =0; i<sizeof(buffer_encrypted_output); i++)
    //fwrite(buffer_encrypted_output, sizeof(unsigned char), AES_BLOCK_SIZE, fptr_output);

    fwrite(buffer_encrypted_output, AES_BLOCK_SIZE, AES_BLOCK_SIZE * num_blocks, fptr_output);

    // // Exibindo a saída criptografada
    // printf("Texto criptografado (em hexadecimal):\n");
    // for (int i = 0; i < num_blocks * AES_BLOCK_SIZE; ++i) {
    //     printf("%02X", buffer_encrypted_output[i]);
    // }
    // printf("\n");

    // Liberando memória alocada
    // free(buffer_encrypted_output);
    // fclose(fptr_output);
    // fclose(arquivo);

    fprintf(fptr_executions, "%d;%s;%d;%f\n",num_blocks, SCHEDULE_TYPE, SET_NUM_THREADS_ARGV, time_spent );
    printf("\nTempo de execução: %f segundos\n", time_spent);
    return 0;
}
