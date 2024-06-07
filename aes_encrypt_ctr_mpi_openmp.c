//  gcc -fopenmp aes_encrypt_ctr.c /home/cp03/tiny-AES-c/aes.c -o aes_encrypt_ctr -I/home/cp03/tiny-AES-c && echo '' &&time ./aes_encrypt_ctr
//  dd if=/dev/zero bs=1M count=512 | tr '\000' '\001' > /dev/shm/large_file.dat
// hexdump -C /dev/shm/large_file.dat
// ladalloc -n 1 -t 10 -s/e

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>
#include <time.h>
#include "mpi.h"
#include "aes.h"

#define AES128 1
#define AES_BLOCK_SIZE 16
#define MESSAGE_BLOCKS 4 //Blocos por mensagem
#define ECB 1
#define CBC 1
#define CTR 1
#define TAG_INIT_SLAVE 1
#define TAG_SLAVE_REQ_WORK 2
#define TAG_MASTER_SEND_WORK 3
#define TAG_SLAVE_RESP_FINAL_WORK 4
#define TAG_FINALIZE 6
#define DEBUG 1

typedef struct
{
    int block_id;
    char message[AES_BLOCK_SIZE]; // Ponteiro para a string
} CustomMessage;

// Função para gerar o próximo bloco de CTR
void generate_ctr_block(unsigned char *nonce, unsigned int counter, unsigned char *output)
{
    // Copia o nonce para o bloco de saída
    memcpy(output, nonce, AES_BLOCK_SIZE);

    // Converte o contador para bytes e copia para o final do bloco de saída
    for (int i = AES_BLOCK_SIZE - sizeof(unsigned int); i < AES_BLOCK_SIZE; ++i)
    {
        output[i] = (counter >> ((AES_BLOCK_SIZE - i - 1) * 8)) & 0xFF;
    }
}

int main(int argc, char *argv[])
{

    int my_rank; // Identificador deste processo
    int proc_n;  // Numero de processos disparados pelo usuário na linha de comando (np)
    // char message[AES_BLOCK_SIZE]; // Buffer para as mensagens
    CustomMessage msg;
    int message_jumbo_blocks;
    message_jumbo_blocks = 1;


    MPI_Status status;

    // Inicializa o MPI
    MPI_Init(&argc, &argv); // funcao que inicializa o MPI, todo o código paralelo esta abaixo

    // Pega o numero do processo atual (rank)
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Pega informação do numero de processos (quantidade total)
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n);

    // int AES_BLOCK_SIZE = message_jumbo_blocks * AES_BLOCK_SIZE;
    // // Alocação dinâmica para a mensagem
    // msg.message = (char *)malloc((AES_BLOCK_SIZE*2) * sizeof(char));
    // if (msg.message == NULL)
    // {
    //     fprintf(stderr, "Erro ao alocar memória msg.message\n");
    //     MPI_Abort(MPI_COMM_WORLD, 1);
    // }
    // printf("size msg.message %lu\n", sizeof(msg.message));
    // fflush(stdout);

    
    // Definindo o tipo MPI para CustomMessage
    const int nitems = 2;
    int blocklengths[2] = {1, AES_BLOCK_SIZE};
    MPI_Datatype types[2] = {MPI_INT, MPI_CHAR};
    MPI_Datatype MPI_CUSTOM_MESSAGE_TYPE;
    MPI_Aint offsets[2];

    offsets[0] = offsetof(CustomMessage, block_id);
    offsets[1] = offsetof(CustomMessage, message);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_CUSTOM_MESSAGE_TYPE);
    MPI_Type_commit(&MPI_CUSTOM_MESSAGE_TYPE);

    if (my_rank != 0) // SLAVE
    {
        char hostname[1024];
        hostname[1023] = '\0';
        gethostname(hostname, 1023);
        printf("Hostpedeira Slave %d: %s\n", my_rank, hostname);
        fflush(stdout);
        if (DEBUG)
            printf("Processo Slave %d inicializando\n", my_rank);
        fflush(stdout);

        // MPI_Send(&request, 0);    // Solicita trabalho ao master
        MPI_Send(&msg, 16, MPI_CUSTOM_MESSAGE_TYPE, 0, TAG_INIT_SLAVE, MPI_COMM_WORLD);

        while (1)
        {

            if (DEBUG)
                printf("Processo Slave %d esperando trabalho\n", my_rank);
            fflush(stdout);

            // MPI_Recv(&response, 0);    // recebo do mestre
            MPI_Recv(&msg, 16, MPI_CUSTOM_MESSAGE_TYPE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (DEBUG)
                printf("Processo Slave %d trabalho recebido\n", my_rank);
            fflush(stdout);

            if (status.MPI_TAG == TAG_FINALIZE)
            {
                printf("Processo Slave %d BREAK!\n", my_rank);
                fflush(stdout);
                break;
            }

            char buffer_in[AES_BLOCK_SIZE];
            memcpy(buffer_in, msg.message, AES_BLOCK_SIZE);

            int i = 0; //<<<<<<<<<<<<<

            //     // Chave AES de 128 bits (16 bytes)
            uint8_t key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
            uint8_t nonce[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

            unsigned char counter_block[AES_BLOCK_SIZE];
            // generate_ctr_block(nonce, i, counter_block);
            generate_ctr_block(nonce, i, counter_block);

            if (DEBUG)
            {
                int counter_block_length = sizeof(counter_block);
                printf("\n counter_block  em hex\n");
                fflush(stdout);

                for (int i = 0; i < counter_block_length; i++)
                    printf("%02X", counter_block[i]);
                fflush(stdout);

                printf("\n");
                fflush(stdout);
            }

            // Criptografa o bloco de contador usando AES ECB
            // uint8_t encrypted_counter_block[] = nonce;

            struct AES_ctx ctx;

            AES_init_ctx(&ctx, key);
            AES_ECB_encrypt(&ctx, counter_block);

            if (DEBUG)
            {
                int counter_block_length = sizeof(counter_block);
                printf("\n counter_block  cifrado\n");
                fflush(stdout);

                for (int i = 0; i < counter_block_length; i++)
                    printf("%02X", counter_block[i]);
                fflush(stdout);

                printf("\n");
                fflush(stdout);
            }

            unsigned char encrypted_output[AES_BLOCK_SIZE];

            // XOR entre o bloco criptografado do contador e o bloco de entrada

            for (int j = 0; j < AES_BLOCK_SIZE; ++j)
            {
                // encrypted_output[i * AES_BLOCK_SIZE + j] = encrypted_counter_block[j] ^ in[i * AES_BLOCK_SIZE + j];
                encrypted_output[AES_BLOCK_SIZE + j] = counter_block[j] ^ buffer_in[j];
            }
            // fprintf(fptr_output, encrypted_output);
            fflush(stdout);

            if (DEBUG)
            {
                printf("\n block cifrado\n");
                fflush(stdout);

                for (int i = 0; i < AES_BLOCK_SIZE; i++)
                    printf("%02X", encrypted_output[i]);
                fflush(stdout);

                printf("\n");
                fflush(stdout);
            }
            // printf(".");
            fflush(stdout);

            if (DEBUG)
                printf("Processo Slave %d respondendo trabalho\n", my_rank);
            fflush(stdout);

            memcpy(msg.message, encrypted_output, AES_BLOCK_SIZE);

            // MPI_Send(&msg, 0);    // retorno resultado para o mestre
            MPI_Send(&msg, 16, MPI_CUSTOM_MESSAGE_TYPE, 0, 0, MPI_COMM_WORLD);

            if (DEBUG)
                printf("Processo Slave %d Send concluído\n", my_rank);
            fflush(stdout);

            // papel do escravo
            if (DEBUG)
                printf("Processo Slave %d solicita trabalho\n", my_rank);
            fflush(stdout);

            // MPI_Send(&request, 0);    // Solicita trabalho ao master
            MPI_Send(&msg, 16, MPI_CUSTOM_MESSAGE_TYPE, 0, TAG_SLAVE_REQ_WORK, MPI_COMM_WORLD);
        }
    }

    else
    {

        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        // MESTRE
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////
        double t1,t2,tf;
        t1 = MPI_Wtime(); 

        char hostname[1024];
        hostname[1023] = '\0';
        gethostname(hostname, 1023);
        printf("Hostpedeira Master: %s\n", hostname);
        FILE *arquivo;
        FILE *fptr_output;
        FILE *fptr_executions;

        char buffer_in[AES_BLOCK_SIZE]; // blocos lidos conforme tamanho do bloco AES
        size_t blocos_lidos;
        long tamanho_arquivo;
        int num_blocks;

        fptr_output = fopen("/dev/shm/output_encrypt.txt", "w");
        fptr_executions = fopen("./output_results.txt", "a");

        // Abre o arquivo para leitura
        arquivo = fopen("/dev/shm/large_file.dat", "rb");
        if (arquivo == NULL)
        {
            perror("Erro ao abrir o arquivo");
            return 1;
        }
        // Obtém o tamanho do arquivo
        fseek(arquivo, 0, SEEK_END);      // Vai para o final do arquivo
        tamanho_arquivo = ftell(arquivo); // Obtém a posição atual (tamanho do arquivo)
        fseek(arquivo, 0, SEEK_SET);      // Volta para o início do arquivo

        // Calcula o número total de blocos
        num_blocks = (int)(tamanho_arquivo / (AES_BLOCK_SIZE - 1));

        if (DEBUG)
            printf("num_blocks %d", num_blocks);
        fflush(stdout);

        // Criando buffer_input para armazenar a leitura do arquivo
        unsigned char *buffer_input_file = (unsigned char *)malloc(AES_BLOCK_SIZE * num_blocks);
        if (!buffer_input_file)
        {
            printf("Erro ao alocar memória. buffer_input_file\n");
            fflush(stdout);
            return 1;
        }

        fread(buffer_input_file, AES_BLOCK_SIZE, num_blocks, arquivo);

        // Criando buffer_encrypted_output para armazenar a saída
        unsigned char *buffer_encrypted_output = (unsigned char *)malloc(AES_BLOCK_SIZE * num_blocks);
        if (!buffer_encrypted_output)
        {
            printf("Erro ao alocar memória.buffer_encrypted_output \n");
            fflush(stdout);

            return 1;
        }

        // Chave AES de 128 bits (16 bytes)
        uint8_t key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
        uint8_t nonce[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

        if (DEBUG)
        {
            int key_length = sizeof(key);
            printf("\n key  em hex\n");
            fflush(stdout);

            for (int i = 0; i < key_length; i++)
                printf("%02hhX", key[i]);
            fflush(stdout);
        }

        if (DEBUG)
        {
            int nonce_length = sizeof(nonce);
            printf("\n Nonce  em hex\n");
            fflush(stdout);

            for (int i = 0; i < nonce_length; i++)
                printf("%02hhX", nonce[i]);
            fflush(stdout);

            printf("\n");
            fflush(stdout);
        }

        int blk_pointer = 0;
        while (1)
        {

            if (blk_pointer >= num_blocks)
            {
                // SHUTDOWN...
                for (int i = 1; i < proc_n; i++)
                {
                    printf("Shutdown proc_n: %d \n", i);
                    fflush(stdout);
                    MPI_Send(&msg, 16, MPI_CUSTOM_MESSAGE_TYPE, i, TAG_FINALIZE, MPI_COMM_WORLD);
                }
                break;
            }

            if (DEBUG)
                printf("Mestre aguardando requisicao\n");
            fflush(stdout);

            MPI_Recv(&msg, 16, MPI_CUSTOM_MESSAGE_TYPE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (DEBUG)
                printf("Mestre recebeu requisicao %d / tag :%d\n", status.MPI_SOURCE, status.MPI_TAG);
            fflush(stdout);

            int recv_tag = status.MPI_TAG;
            if (recv_tag == TAG_INIT_SLAVE || recv_tag == TAG_SLAVE_REQ_WORK)
            {

                // fread(buffer_in, 1, AES_BLOCK_SIZE, arquivo); lemos todo arquivo em buffer_input
                memcpy(buffer_in, buffer_input_file + blk_pointer, AES_BLOCK_SIZE);

                if (DEBUG)
                {
                    int input_length = sizeof(buffer_in);
                    printf("\n buffer_in  em hex\n");
                    fflush(stdout);

                    for (int i = 0; i < input_length; i++)
                        printf("%02hhX", buffer_in[i]);
                    fflush(stdout);
                }
                printf("recv_tag\n");
                memcpy(msg.message, buffer_in, AES_BLOCK_SIZE);
                msg.block_id = blk_pointer;

                printf("recv_tag");
                fflush(stdout);

                // MPI_Send(&msg, status.MPI_SOURCE);
                MPI_Send(&msg, 16, MPI_CUSTOM_MESSAGE_TYPE, status.MPI_SOURCE, TAG_MASTER_SEND_WORK, MPI_COMM_WORLD);
                if (DEBUG)
                    printf("Mestre enviou trabalho\n");
                fflush(stdout);
            }
            else if (recv_tag == TAG_SLAVE_RESP_FINAL_WORK)
            {

                MPI_Recv(&msg, 16, MPI_CUSTOM_MESSAGE_TYPE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                if (DEBUG)
                    printf("Mestre recebeu trabalho processado %d\n", status.MPI_SOURCE);
                fflush(stdout);

                memcpy(buffer_in, msg.message, AES_BLOCK_SIZE);

                int blk_id = msg.block_id;
                for (int j = 0; j < AES_BLOCK_SIZE; ++j)
                {
                    // buffer_encrypted_output[i * AES_BLOCK_SIZE + j] = encrypted_counter_block[j] ^ in[i * AES_BLOCK_SIZE + j];
                    buffer_encrypted_output[blk_id * AES_BLOCK_SIZE + j] = buffer_in[j];
                }
                // fprintf(fptr_output, buffer_encrypted_output);
                fflush(stdout);
            }
            blk_pointer++;
        }
        t2 = MPI_Wtime(); // termina a contagem do tempo
        tf = t2-t1;
        printf("\nTempo de execucao: %f\n\n", tf); 
        // // Obter o tempo final
        // clock_gettime(CLOCK_MONOTONIC, &end);
        // // Calcular o tempo decorrido
        // time_spent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        // for(int i =0; i<sizeof(buffer_encrypted_output); i++)
        //fwrite(buffer_encrypted_output, sizeof(unsigned char), AES_BLOCK_SIZE, fptr_output);

        fwrite(buffer_encrypted_output, AES_BLOCK_SIZE, AES_BLOCK_SIZE * num_blocks, fptr_output);

        fprintf(fptr_executions, "%d;%f\n", num_blocks, tf);
        fflush(stdout);

        fflush(stdout);
    }

    MPI_Finalize();
    return 0;
}
