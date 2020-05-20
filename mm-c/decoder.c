#include "decoder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

// constants
static const uint8_t ZERO = '0';
static const uint8_t PREFIX_LIST[] =
{
186, 158, 135, 159,
136, 150, 137, 138,
187, 151, 182, 152,
139, 183, 188, 134,
185, 189, 180, 157,
155, 156, 131, 132,
133, 130, 181, 176,
177, 153, 184, 178,
173, 147, 175, 199,
166, 170, 198, 171,
191, 145, 165, 172, 
154, 146
};
static const size_t PREFIX_SIZE = sizeof(PREFIX_LIST)/sizeof(PREFIX_LIST[0]);

// global variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
Decoder decoder;

void quick_sort(SortedMobileHash *array, int from, int to)
{
    if(from>=to)return;
    SortedMobileHash temp;
    int i = from, j;
    for(j = from + 1;j <= to;j++)
    {
        if(is_lesser(&array[j].mobile_hash,&array[from].mobile_hash))
        {
            i = i + 1;
            temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
    }
    
    temp = array[i];
    array[i] = array[from];
    array[from] = temp;
    quick_sort(array,from,i-1);
    quick_sort(array,i+1,to);
}

inline int binary_search(SortedMobileHash *array, int len, const MobileHash* key)
{
    int low = 0, high = len-1, mid;
    while(low <= high)
    {
        mid = (low + high)/2;
        if(is_equal(&array[mid].mobile_hash, key))
        {
            return mid;
        }
        else if(is_lesser(&array[mid].mobile_hash, key))
        {
            low = mid + 1; 
        }
        else
        {
            high = mid - 1;
        }
    }
    return -1;
}

inline char hexToNibble(char n)
{
    return n - ( n <= '9' ? '0' : ('a'-10) );
}
void hex_to_bytes(uint8_t* to, char* from, int len)
{
    for(int i = 0; i < len/2; i++)
    {
        to[i] = (hexToNibble(from[i*2])<<4) + hexToNibble(from[i*2+1]);
    }
}

void print_mobile_hash(MobileHash * mh) {
    for (size_t i = 0; i < STATE_LEN; i++)
    {
        printf("%x-", mh->hash[i]);
    }
    printf(",");
    for (size_t i = 0; i < MOBILE_LEN; i++)
    {
        printf("%c", mh->mobile[i]);
    }
    printf("\n");
}
void print_sorted_mobile_hash(SortedMobileHash* smh) {
    printf("%d,%d,", smh->index, smh->index_dup);
    print_mobile_hash(&smh->mobile_hash);
}

size_t validate_hash_string(const char* s)
{
    int valid_char_num = 0;
    int count = 0;
    while (*s)
    {
        if(*s == ',')
        {
            if(valid_char_num == HASH_LEN)
            {
                count++;
            }
            valid_char_num = 0;
        } 
        else if ((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9'))
        {
            valid_char_num ++;
        }
        s++;
    }
    if(valid_char_num == HASH_LEN)
    {
        count++;
    }
    
    return count;
}

void parse_hash_strings(const char* s)
{
    SortedMobileHash* p_smh = decoder.s_mobile_hash;
    HashString *p_hs = decoder.hash_string;

    int valid_char_num = 0;
    int count = 0;
    char hash_string[HASH_LEN];
    while (*s)
    {
        if(*s == ',')
        {
            if(valid_char_num == HASH_LEN)
            {
                memcpy(p_hs,hash_string,HASH_LEN);
                hex_to_bytes((uint8_t*)p_smh->mobile_hash.hash, hash_string, HASH_LEN);
                p_smh->index = count;
                count++;
                p_smh++;
                p_hs++;
            }
            valid_char_num = 0;
        } 
        else if ((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9'))
        {
            hash_string[valid_char_num] = *s;
            valid_char_num ++;
        }
        s++;
    }
    if(valid_char_num == HASH_LEN)
    {
        memcpy(p_hs,hash_string,HASH_LEN);
        hex_to_bytes((uint8_t*)p_smh->mobile_hash.hash, hash_string, HASH_LEN);
        p_smh->index = count;    
    }
}

void dedup_sorted_mobile_hash()
{
    decoder.dedup_len = decoder.hash_len;
    for (size_t i = 1; i < decoder.dedup_len; i++)
    {
        if (is_equal(&decoder.s_mobile_hash[i].mobile_hash, &decoder.s_mobile_hash[i-1].mobile_hash))
        {
            SortedMobileHash temp = decoder.s_mobile_hash[i];
            temp.index_dup = decoder.s_mobile_hash[i-1].index;
            for (size_t j = i; j < decoder.hash_len - 1; j++)
            {
                decoder.s_mobile_hash[j] = decoder.s_mobile_hash[j+1];
            }
            decoder.s_mobile_hash[decoder.hash_len-1] = temp;
            decoder.dedup_len--;
            i--;
        }        
    }
}

Decoder* init_decoder(const char* s)
{
    decoder.prefix_bytes = calloc(PREFIX_SIZE, sizeof(B3));
    for (int i = 0; i < PREFIX_SIZE; i++)
    {
        decoder.prefix_bytes[i].b1 = PREFIX_LIST[i]/100 % 10 + ZERO;
        decoder.prefix_bytes[i].b2 = PREFIX_LIST[i]/10 % 10 + ZERO;
        decoder.prefix_bytes[i].b3 = PREFIX_LIST[i] % 10 + ZERO;
    }
    decoder.hash_len = validate_hash_string(s);
    decoder.hash_string = calloc(decoder.hash_len, sizeof(HashString));
    decoder.mobile_hash = calloc(decoder.hash_len, sizeof(MobileHash));
    decoder.s_mobile_hash = calloc(decoder.hash_len, sizeof(SortedMobileHash));
    decoder.count = 0;
    parse_hash_strings(s);
    quick_sort(decoder.s_mobile_hash, 0, decoder.hash_len-1);
    dedup_sorted_mobile_hash();
    // for (size_t i = 0; i < decoder.hash_len; i++)
    // {
    //     printf("%zu,", i);
    //     print_sorted_mobile_hash(decoder.s_mobile_hash+i);
    // }
    
    return &decoder;
}

void free_decoder()
{
    free(decoder.prefix_bytes);
    decoder.prefix_bytes = 0;
    free(decoder.hash_string);
    decoder.hash_string = 0;
    free(decoder.mobile_hash);
    decoder.mobile_hash = 0;
    free(decoder.s_mobile_hash);
    decoder.s_mobile_hash = 0;
}

void decode(size_t thread_num, size_t threadid)
{
    MobileHash *mh = alloc_mh();
    uint8_t *mobile = mh->mobile;
    SortedMobileHash *smh = decoder.s_mobile_hash;
    int dedup_len = decoder.dedup_len;
    int count = 0;

    // each prefix
    for (size_t i = threadid; i < PREFIX_SIZE; i+=thread_num)
    {
        memcpy(mobile, decoder.prefix_bytes+i, 3);
        // each number
        for (uint8_t n1 = 0; n1 < 10; n1++)
        {
            mobile[3] = n1 + ZERO;
            for (uint8_t n2 = 0; n2 < 10; n2++)
            {
                mobile[4] = n2 + ZERO;
                for (uint8_t n3 = 0; n3 < 10; n3++)
                {
                    mobile[5] = n3 + ZERO;
                    for (uint8_t n4 = 0; n4 < 10; n4++)
                    {
                        mobile[6] = n4 + ZERO;
                        for (uint8_t n5 = 0; n5 < 10; n5++)
                        {
                            mobile[7] = n5 + ZERO;
                            for (uint8_t n6 = 0; n6 < 10; n6++)
                            {
                                mobile[8] = n6 + ZERO;
                                for (uint8_t n7 = 0; n7 < 10; n7++)
                                {
                                    mobile[9] = n7 + ZERO;
                                    for (uint8_t n8 = 0; n8 < 10; n8++)
                                    {
                                        mobile[10] = n8 + ZERO;
                                        md5_hash(mh);                                        
                                        int index = binary_search(smh, dedup_len, mh);
                                        if (index >= 0)
                                        {
                                            smh[index].mobile_hash = *mh;
                                            count++;
                                        }                                              
                                    }
                                }
                            }
                        }
                    }
                }
            }
            pthread_mutex_lock(&mutex);
            decoder.count += count;
            int finished = (decoder.count == dedup_len);
            pthread_mutex_unlock(&mutex);
            count = 0;
            if(finished)
            {
                goto end;
            }
        }
    }

end:
    free(mh);
    mh = 0;
}

void* thread_computing(void* p)
{
    ThreadInfo* ti = (ThreadInfo*) p;
    decode(ti->thread_num, ti->threadid);
    return 0;
}

void* thread_printing(void* p)
{
    time_t start = time(NULL);
    printf("0/%zu\n", decoder.dedup_len);
    while (1) {
        sleep(1);
        printf("\033[1A%zu/%zu @%lds\n", decoder.count, decoder.dedup_len, time(NULL) - start);
    }
}

void resort_mobile_hash()
{
    for (int i = 0; i < decoder.hash_len; i++)
    {
        if (i < decoder.dedup_len)
        {
            int index = decoder.s_mobile_hash[i].index;
            decoder.mobile_hash[index] = decoder.s_mobile_hash[i].mobile_hash;            
        }
        else
        {
            int index = decoder.s_mobile_hash[i].index;
            int index_dup = decoder.s_mobile_hash[i].index_dup;
            decoder.mobile_hash[index] = decoder.mobile_hash[index_dup];
        }
    }
}

