#include <stdlib.h>
#include <string.h>
#include "md5_hash.h"

#define LENGTH_SIZE 8 // In bytes
MobileHash *alloc_mh()
{
	MobileHash *mh = malloc(sizeof(MobileHash));
	// initialize padding
	memset(mh->mobile, 0, BLOCK_LEN);
	mh->mobile[MOBILE_LEN] = 0x80;
	mh->mobile[BLOCK_LEN - LENGTH_SIZE] = 'X';
	return mh;
}

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
