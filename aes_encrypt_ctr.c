#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <omp.h> 
#include <time.h>


#define AES128 1
#define AES_BLOCK_SIZE 16
#define ECB 1
#define CBC 1
#define CTR 1
#define DEBUG 0


#include "aes.h"


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
    if(argc < 3){
        printf("\n SCHEDULED and SET_NUM_THREADS_ARGV required\n");
        return 0;
    }


    char* SCHEDULE_TYPE = argv[1];
    int SET_NUM_THREADS_ARGV = atoi(argv[2]);
    int chunk_size = 1;
    
    omp_sched_t omp_schedule;    
    if (strcmp(SCHEDULE_TYPE, "static") == 0) {
        omp_schedule = omp_sched_static;
    } else if (strcmp(SCHEDULE_TYPE, "dynamic") == 0) {
        omp_schedule = omp_sched_dynamic;
    } else if (strcmp(SCHEDULE_TYPE, "guided") == 0) {
        omp_schedule = omp_sched_guided;
    } else if (strcmp(SCHEDULE_TYPE, "auto") == 0) {
        omp_schedule = omp_sched_auto;
    } else {
        printf("Invalid schedule type\n");
        return 1;
    }

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

    // Criando buffer_in para armazenar a saída
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


    struct timespec start, end;
    double time_spent;
    clock_gettime(CLOCK_MONOTONIC, &start);

    omp_set_num_threads(SET_NUM_THREADS_ARGV);
    omp_set_schedule(omp_schedule,chunk_size);
    // Criptografando cada bloco de entrada usando CTR
    #pragma omp parallel for schedule(runtime)
    for (int i = 0; i < num_blocks; ++i) {
        
        fread(buffer_in, 1,AES_BLOCK_SIZE, arquivo);
        if(DEBUG)
        {
            int input_length = sizeof(buffer_in);    
            printf("\n in  em hex\n");
            for(int i = 0; i < input_length; i++)
                printf("%02hhX", buffer_in[i]);
        }

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

    fwrite(buffer_encrypted_output, AES_BLOCK_SIZE, AES_BLOCK_SIZE * num_blocks, fptr_output);

    // Exibindo a saída criptografada
    if(DEBUG){
        printf("Texto criptografado (em hexadecimal):\n");
        for (int i = 0; i < num_blocks * AES_BLOCK_SIZE; ++i) {
            printf("%02X", buffer_encrypted_output[i]);
        }
        printf("\n");
     }

    // Liberando memória alocada
    // free(buffer_encrypted_output);
    // fclose(fptr_output);
    // fclose(arquivo);

    fprintf(fptr_executions, "%d;%s;%d;%f\n",num_blocks, SCHEDULE_TYPE, SET_NUM_THREADS_ARGV, time_spent );
    printf("\nTempo de execução: %f segundos\n", time_spent);
    return 0;
}
