#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define HASH_LEN 32
#define MOBILE_LEN 11
#define BLOCK_LEN 64 // In bytes
#define STATE_LEN 4  // In words

typedef char HashString[HASH_LEN];

typedef struct
{
    uint32_t value[STATE_LEN];
} Hash;

typedef struct
{
    uint8_t value[MOBILE_LEN];
} MobileData;

typedef struct
{
    int index;
    int index_dup;
    Hash hash;
} SortedHash;

typedef struct
{
    int hash_len;
    int dedup_len;
    HashString* hash_string;
    SortedHash* s_hash;
    MobileData* m_data;
} Decoder;

void init_decoder(Decoder* decoder, const char* s);
void free_decoder(Decoder* decoder);
void resort_data(Decoder* decoder, MobileData* p_m_data);