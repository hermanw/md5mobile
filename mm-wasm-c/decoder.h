#include <stddef.h>
#include <stdint.h>
#include "md5_hash.h"

#define HASH_LEN 32

typedef struct
{
    int        index;
    char       hash_string[HASH_LEN];
    MobileHash mh;
} MD5_MOBILE;

typedef struct
{
    MD5_MOBILE* md5_mobile;
    size_t hash_len;
    size_t thread_num;
    size_t threadid;
} THREAD_INFO;

int _found();
int is_equal(const MobileHash *a, const MobileHash *b);
void print_md5_mobile(MD5_MOBILE* mm);
size_t prep_data(const char* s, MD5_MOBILE** p);
void* thread_f(void* p);