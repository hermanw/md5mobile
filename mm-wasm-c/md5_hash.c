#include <stdlib.h>
#include <string.h>
#include "md5_hash.h"

// Link this program with an external C or x86 compression function
extern void md5_compress(uint32_t state[static STATE_LEN], const uint8_t block[static BLOCK_LEN]);

struct MobileHash* alloc_mh() {
	struct MobileHash* mh = malloc(sizeof(struct MobileHash));
	// initialize padding
	#define LENGTH_SIZE 8  // In bytes
	memset(mh->mobile, 0, BLOCK_LEN);
	mh->mobile[MOBILE_LEN] = 0x80;
	mh->mobile[BLOCK_LEN - LENGTH_SIZE] = 'X';
	return mh;
}

void md5_hash(struct MobileHash *mh) {
	mh->hash[0] = UINT32_C(0x67452301);
	mh->hash[1] = UINT32_C(0xEFCDAB89);
	mh->hash[2] = UINT32_C(0x98BADCFE);
	mh->hash[3] = UINT32_C(0x10325476);
	
	md5_compress(mh->hash, mh->mobile);
}