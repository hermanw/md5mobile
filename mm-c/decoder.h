#include <stddef.h>
#include <stdint.h>
#include "md5_hash.h"

#define HASH_LEN 32

typedef struct
{
    size_t thread_num;
    size_t threadid;
} ThreadInfo;

typedef struct
{
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
} B3;

typedef struct
{
    int index;
    int index_dup;
    MobileHash mobile_hash;
} SortedMobileHash;

typedef char HashString[HASH_LEN];
typedef struct
{
    B3 *prefix_bytes;
    HashString *hash_string;
    size_t hash_len;
    MobileHash *mobile_hash;
    SortedMobileHash *s_mobile_hash;
    size_t dedup_len;
    size_t count;
} Decoder;

Decoder *init_decoder(const char *s);
void free_decoder();
void *thread_computing(void *p);
void *thread_printing(void *p);
void resort_mobile_hash();
