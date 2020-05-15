#include <stddef.h>
#include <stdint.h>

// this file is specific to hash a 11 chars long mobile number string
#define MOBILE_LEN 11
#define BLOCK_LEN 64 // In bytes
#define STATE_LEN 4  // In words

typedef struct
{
    uint8_t mobile[BLOCK_LEN];
    uint32_t hash[STATE_LEN];
} MobileHash;

MobileHash *alloc_mh();
void md5_hash(MobileHash *mh);
