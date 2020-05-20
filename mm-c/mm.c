#include "decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

char* read_from_file(char* filename)
{
    FILE * f = fopen (filename, "rb");
    if (f)
    {
        fseek (f, 0, SEEK_END);
        int len = ftell (f);
        fseek (f, 0, SEEK_SET);
        char * buffer = malloc (len + 1);
        fread (buffer, 1, len, f);
        fclose (f);
        buffer[len] = 0;
        return buffer;
    }
    return 0;
}

void write_to_file (char* filename, Decoder* decoder)
{
    FILE * f = fopen (filename, "w");
    for (int h = 0; h < decoder->hash_len; h++)
    {
        for (size_t i = 0; i < HASH_LEN; i++)
        {
            fputc(decoder->hash_string[h][i], f);
        }
        fputc(',', f);
        for (size_t i = 0; i < MOBILE_LEN; i++)
        {
            fputc(decoder->mobile_hash[h].mobile[i], f);
        }
        fputc('\n', f);
    }
    fclose (f);
}

int main(int argc, char *argv[]) {
    char *s = NULL;
    if (argc < 2 || !(s = read_from_file(argv[1])))
    {
        printf("mobile md5 decoder, v1.1, by herman\n");
        printf("usage: mm filename\n");
        return 0;
    }
    
    Decoder* decoder = init_decoder(s);
    free(s);
    s = 0;
    printf("find %zu hashes\n", decoder->hash_len);
    printf("they have %zu duplicated, %zu unique ones\n", decoder->hash_len-decoder->dedup_len, decoder->dedup_len);

    size_t thread_num = sysconf(_SC_NPROCESSORS_ONLN);
    printf("starting %lu threads...\n", thread_num);

    pthread_t threads[thread_num];
    ThreadInfo ti[thread_num];
    for (size_t i = 0; i <thread_num; i++)
    {
        ti[i].thread_num = thread_num;
        ti[i].threadid = i;
        pthread_create(&threads[i], 0, thread_computing, (void*) &ti[i]);
    }

    pthread_t thread_p;
    pthread_create(&thread_p, 0, thread_printing, 0);

    for (size_t i = 0; i <thread_num; i++)
    {
        pthread_join(threads[i], NULL);
    }
    pthread_cancel(thread_p);

    printf("total %zu hashes are decode\n", decoder->count);
    resort_mobile_hash();
    char outfile[strlen(argv[1])+4];
    strcpy(outfile, argv[1]);
    strcat(outfile,".out");
    write_to_file(outfile, decoder);
    printf("please find results in file: %s\n", outfile);

    free_decoder();
    return 0;
}