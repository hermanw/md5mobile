#include "decoder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

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

typedef struct
{
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
} B3;

int is_equal(const MobileHash *a, const MobileHash *b)
{
    if (a->hash[0] == b->hash[0]
        && a->hash[1] == b->hash[1]
        && a->hash[2] == b->hash[2]
        && a->hash[3] == b->hash[3])
        return 1;
    
    return 0;
}

int is_lesser(const MobileHash *a, const MobileHash *b)
{
    if (a->hash[0] < b->hash[0])
    {
        return 1;
    } else if (a->hash[0] == b->hash[0]) {
        if (a->hash[1] < b->hash[1])
        {
            return 1;
        } else if (a->hash[1] == b->hash[1]) {
            if (a->hash[2] < b->hash[2])
            {
                return 1;
            } else if (a->hash[2] == b->hash[2]) {
                if (a->hash[3] < b->hash[3])
                {
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

void quick_sort(MD5_MOBILE *array, int from, int to)
{
    if(from>=to)return;
    MD5_MOBILE temp;
    int i = from, j;
    for(j = from + 1;j <= to;j++)
    {
        if(is_lesser(&array[j].mh,&array[from].mh))
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

int binary_search(MD5_MOBILE *array, int len, const MobileHash* key)
{
    int low = 0, high = len-1, mid;
    while(low <= high)
    {
        mid = (low + high)/2;
        if(is_equal(&array[mid].mh, key))
        {
            return mid;
        }
        else if(is_lesser(&array[mid].mh, key))
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
void print_md5_mobile(MD5_MOBILE* mm) {
    printf("%d,", mm->index);
    for (size_t i = 0; i < HASH_LEN; i++)
    {
        printf("%c", mm->hash_string[i]);
    }
    printf(",");
    print_mobile_hash(&mm->mh);
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

MD5_MOBILE* parse_hash_strings(const char* s, size_t len)
{
    MD5_MOBILE* mm = calloc(len, sizeof(MD5_MOBILE));
    MD5_MOBILE* p = mm;

    int valid_char_num = 0;
    int count = 0;
    char hash_string[HASH_LEN];
    while (*s)
    {
        if(*s == ',')
        {
            if(valid_char_num == HASH_LEN)
            {
                memcpy(p->hash_string,hash_string,HASH_LEN);
                hex_to_bytes((uint8_t*)p->mh.hash, hash_string, HASH_LEN);
                p->index = count;
                count++;
                p++;
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
        memcpy(p->hash_string,hash_string,HASH_LEN);
        hex_to_bytes((uint8_t*)p->mh.hash, hash_string, HASH_LEN);
        p->index = count;    
    }

    return mm;
}

size_t prep_data(const char* s, MD5_MOBILE** p)
{
    size_t hash_len = validate_hash_string(s);
    MD5_MOBILE* md5_mobile = parse_hash_strings(s, hash_len);
    quick_sort(md5_mobile, 0, hash_len-1);
    *p = md5_mobile;
    return hash_len;
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int found = 0;
time_t start = 0;

inline int _found() {return found;}

void decode(MD5_MOBILE* md5_mobile, size_t hash_len, size_t thread_num, size_t threadid)
{
    // init decoder
    MobileHash* mh = alloc_mh();
    // alloc and fill prefix bytes
    size_t prefix_size = sizeof(PREFIX_LIST)/sizeof(PREFIX_LIST[0]);
    B3 * prefix_bytes = calloc(prefix_size, sizeof(B3));
    for (int i = 0; i < prefix_size; i++)
    {
        prefix_bytes[i].b1 = PREFIX_LIST[i]/100 % 10 + ZERO;
        prefix_bytes[i].b2 = PREFIX_LIST[i]/10 % 10 + ZERO;
        prefix_bytes[i].b3 = PREFIX_LIST[i] % 10 + ZERO;
    }
    int count = 0;
    int finished = 0;
    if(!start) {start = time(NULL);}

    // each prefix
    for (size_t i = threadid; i < prefix_size; i+=thread_num)
    {
        memcpy(mh->mobile, prefix_bytes+i, 3);
        // each number
        for (uint8_t n1 = 0; n1 < 10; n1++)
        {
            mh->mobile[3] = n1 + ZERO;
            for (uint8_t n2 = 0; n2 < 10; n2++)
            {
                mh->mobile[4] = n2 + ZERO;
                for (uint8_t n3 = 0; n3 < 10; n3++)
                {
                    mh->mobile[5] = n3 + ZERO;
                    for (uint8_t n4 = 0; n4 < 10; n4++)
                    {
                        mh->mobile[6] = n4 + ZERO;
                        for (uint8_t n5 = 0; n5 < 10; n5++)
                        {
                            mh->mobile[7] = n5 + ZERO;
                            for (uint8_t n6 = 0; n6 < 10; n6++)
                            {
                                mh->mobile[8] = n6 + ZERO;
                                for (uint8_t n7 = 0; n7 < 10; n7++)
                                {
                                    mh->mobile[9] = n7 + ZERO;
                                    for (uint8_t n8 = 0; n8 < 10; n8++)
                                    {
                                        mh->mobile[10] = n8 + ZERO;
                                        md5_hash(mh);
                                        int index = binary_search(md5_mobile, hash_len, mh);
                                        if (index >= 0)
                                        {
                                            md5_mobile[index].mh = *mh;
                                            count++;                                        
                                        }                                        
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (n1 & 1) // only a half chance to do
            {
                pthread_mutex_lock(&mutex);
                found += count;
                if (found == hash_len) finished = 1;
                if (count > 0)
                {
                    count = 0;
                    printf("%d/%zu @%lds\n", found, hash_len, time(NULL) - start);
                }            
                pthread_mutex_unlock(&mutex);
                if (finished)
                {
                    goto end;
                }            
            }            
        }
    }

end:
    // release_decoder
    free(mh);
    mh = 0;
    free(prefix_bytes);
    prefix_bytes = 0;
}

void* thread_f(void* p)
{
    THREAD_INFO* ti = (THREAD_INFO*) p;
    decode(ti->md5_mobile, ti->hash_len, ti->thread_num, ti->threadid);
    return 0;
}
