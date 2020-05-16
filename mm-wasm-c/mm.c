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

void write_to_file (char* filename, MD5_MOBILE* md5_mobile, int hash_len)
{
    FILE * f = fopen (filename, "w");
    for (int h = 0; h <hash_len; h++)
    {
        for (size_t i = 0; i < HASH_LEN; i++)
        {
            fputc(md5_mobile[h].hash_string[i], f);
        }
        fputc(',', f);
        for (size_t i = 0; i < MOBILE_LEN; i++)
        {
            fputc(md5_mobile[h].mh.mobile[i], f);
        }
        fputc('\n', f);
    }
    fclose (f);
}

void sort_md5_mobile(MD5_MOBILE** md5_mobile, int hash_len)
{
    MD5_MOBILE* temp = calloc(hash_len, sizeof(MD5_MOBILE));
    for (int i = 0; i < hash_len; i++)
    {
        int index = (*md5_mobile)[i].index;
        temp[index] = (*md5_mobile)[i];
    }
    free(*md5_mobile);
    *md5_mobile = temp;
}

int main(int argc, char *argv[]) {
    char *s = NULL;
    if (argc < 2 || !(s = read_from_file(argv[1])))
    {
        printf("usage: mm filename\n");
        return 0;
    }
    
    MD5_MOBILE* md5_mobile = 0;
    int hash_len =  prep_data(s, &md5_mobile);
    printf("find %d hashes\n", hash_len);
    free(s);
    s = 0;

    int thread_num = sysconf(_SC_NPROCESSORS_ONLN);
    printf("starting %d threads...\n", thread_num);

    pthread_t threads[thread_num];
    THREAD_INFO ti[thread_num];
    for (int i = 0; i <thread_num; i++)
    {
        ti[i].md5_mobile = md5_mobile;
        ti[i].thread_num = thread_num;
        ti[i].threadid = i;
        ti[i].hash_len = hash_len;
        pthread_create(&threads[i], 0, thread_f, (void*) &ti[i]);
    }
    for (int i = 0; i <thread_num; i++)
    {
        pthread_join(threads[i], NULL);
    }

    char outfile[strlen(argv[1])+4];
    strcpy(outfile, argv[1]);
    strcat(outfile,".out");
    sort_md5_mobile(&md5_mobile, hash_len);
    write_to_file(outfile, md5_mobile, hash_len);
    printf("please find results in file: %s\n", outfile);
    // for (int i = 0; i <hash_len; i++)
    // {
    //     print_md5_mobile(md5_mobile+i);
    // }

    free(md5_mobile);
    md5_mobile = 0;
    return 0;
}