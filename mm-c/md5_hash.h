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
int is_equal(const MobileHash *a, const MobileHash *b);
int is_lesser(const MobileHash *a, const MobileHash *b);

// Link this program with an external C or x86 compression function
extern void md5_compress(uint32_t state[static STATE_LEN], const uint8_t block[static BLOCK_LEN]);

inline void md5_hash(MobileHash *mh)
{
	mh->hash[0] = UINT32_C(0x67452301);
	mh->hash[1] = UINT32_C(0xEFCDAB89);
	mh->hash[2] = UINT32_C(0x98BADCFE);
	mh->hash[3] = UINT32_C(0x10325476);

	md5_compress(mh->hash, mh->mobile);
}
