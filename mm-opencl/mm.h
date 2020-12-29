static const char *const compute_cl = R"(
#define BLOCK_LEN 64 // In bytes
#define STATE_LEN 4  // In words
#define LENGTH_SIZE 8 // In bytes
#define MOBILE_LEN 11
#define SLICE_LEN 100000000

#define DATA_TYPE_MOBILE 0
#define DATA_TYPE_CNID 1

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

typedef struct
{
    uint8_t data[BLOCK_LEN];
    uint32_t hash[STATE_LEN];
} DataHash;

typedef struct
{
    int index;
    int index_dup;
    DataHash data_hash;
} SortedDataHash;

static int compare_hash(__global const uint32_t* a, const uint32_t* b)
{
    for (int i = 0; i < STATE_LEN; i++)
    {
        if (a[i] < b[i])
        {
            return -1;
        }
        else if (a[i] > b[i])
        {
            return 1;
        }        
    }

    return 0;
}

static int binary_search(__global SortedDataHash* array, int len, const uint32_t* hash)
{
    int low = 0, high = len - 1, mid;
    while (low <= high)
    {
        mid = (low + high) / 2;
        int r = compare_hash(array[mid].data_hash.hash, hash);
        if (r == 0)
        {
            return mid;
        }
        else if (r < 0)
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

static void md5_compress(uint32_t state[4], const uint8_t block[64])
{
    unsigned int schedule[16];
    schedule[0] = (unsigned int)block[0 * 4 + 0] << 0 | (unsigned int)block[0 * 4 + 1] << 8 | (unsigned int)block[0 * 4 + 2] << 16 | (unsigned int)block[0 * 4 + 3] << 24;
    schedule[1] = (unsigned int)block[1 * 4 + 0] << 0 | (unsigned int)block[1 * 4 + 1] << 8 | (unsigned int)block[1 * 4 + 2] << 16 | (unsigned int)block[1 * 4 + 3] << 24;
    schedule[2] = (unsigned int)block[2 * 4 + 0] << 0 | (unsigned int)block[2 * 4 + 1] << 8 | (unsigned int)block[2 * 4 + 2] << 16 | (unsigned int)block[2 * 4 + 3] << 24;
    schedule[3] = (unsigned int)block[3 * 4 + 0] << 0 | (unsigned int)block[3 * 4 + 1] << 8 | (unsigned int)block[3 * 4 + 2] << 16 | (unsigned int)block[3 * 4 + 3] << 24;
    schedule[4] = (unsigned int)block[4 * 4 + 0] << 0 | (unsigned int)block[4 * 4 + 1] << 8 | (unsigned int)block[4 * 4 + 2] << 16 | (unsigned int)block[4 * 4 + 3] << 24;
    schedule[5] = (unsigned int)block[5 * 4 + 0] << 0 | (unsigned int)block[5 * 4 + 1] << 8 | (unsigned int)block[5 * 4 + 2] << 16 | (unsigned int)block[5 * 4 + 3] << 24;
    schedule[6] = (unsigned int)block[6 * 4 + 0] << 0 | (unsigned int)block[6 * 4 + 1] << 8 | (unsigned int)block[6 * 4 + 2] << 16 | (unsigned int)block[6 * 4 + 3] << 24;
    schedule[7] = (unsigned int)block[7 * 4 + 0] << 0 | (unsigned int)block[7 * 4 + 1] << 8 | (unsigned int)block[7 * 4 + 2] << 16 | (unsigned int)block[7 * 4 + 3] << 24;
    schedule[8] = (unsigned int)block[8 * 4 + 0] << 0 | (unsigned int)block[8 * 4 + 1] << 8 | (unsigned int)block[8 * 4 + 2] << 16 | (unsigned int)block[8 * 4 + 3] << 24;
    schedule[9] = (unsigned int)block[9 * 4 + 0] << 0 | (unsigned int)block[9 * 4 + 1] << 8 | (unsigned int)block[9 * 4 + 2] << 16 | (unsigned int)block[9 * 4 + 3] << 24;
    schedule[10] = (unsigned int)block[10 * 4 + 0] << 0 | (unsigned int)block[10 * 4 + 1] << 8 | (unsigned int)block[10 * 4 + 2] << 16 | (unsigned int)block[10 * 4 + 3] << 24;
    schedule[11] = (unsigned int)block[11 * 4 + 0] << 0 | (unsigned int)block[11 * 4 + 1] << 8 | (unsigned int)block[11 * 4 + 2] << 16 | (unsigned int)block[11 * 4 + 3] << 24;
    schedule[12] = (unsigned int)block[12 * 4 + 0] << 0 | (unsigned int)block[12 * 4 + 1] << 8 | (unsigned int)block[12 * 4 + 2] << 16 | (unsigned int)block[12 * 4 + 3] << 24;
    schedule[13] = (unsigned int)block[13 * 4 + 0] << 0 | (unsigned int)block[13 * 4 + 1] << 8 | (unsigned int)block[13 * 4 + 2] << 16 | (unsigned int)block[13 * 4 + 3] << 24;
    schedule[14] = (unsigned int)block[14 * 4 + 0] << 0 | (unsigned int)block[14 * 4 + 1] << 8 | (unsigned int)block[14 * 4 + 2] << 16 | (unsigned int)block[14 * 4 + 3] << 24;
    schedule[15] = (unsigned int)block[15 * 4 + 0] << 0 | (unsigned int)block[15 * 4 + 1] << 8 | (unsigned int)block[15 * 4 + 2] << 16 | (unsigned int)block[15 * 4 + 3] << 24;

    unsigned int a = state[0];
    unsigned int b = state[1];
    unsigned int c = state[2];
    unsigned int d = state[3];

    a = 0U + a + (d ^ (b & (c ^ d))) + (0xD76AA478U) + schedule[0]; a = 0U + b + (((0U + (a)) << (7)) | ((a) >> (32 - (7))));
    d = 0U + d + (c ^ (a & (b ^ c))) + (0xE8C7B756U) + schedule[1]; d = 0U + a + (((0U + (d)) << (12)) | ((d) >> (32 - (12))));
    c = 0U + c + (b ^ (d & (a ^ b))) + (0x242070DBU) + schedule[2]; c = 0U + d + (((0U + (c)) << (17)) | ((c) >> (32 - (17))));
    b = 0U + b + (a ^ (c & (d ^ a))) + (0xC1BDCEEEU) + schedule[3]; b = 0U + c + (((0U + (b)) << (22)) | ((b) >> (32 - (22))));
    a = 0U + a + (d ^ (b & (c ^ d))) + (0xF57C0FAFU) + schedule[4]; a = 0U + b + (((0U + (a)) << (7)) | ((a) >> (32 - (7))));
    d = 0U + d + (c ^ (a & (b ^ c))) + (0x4787C62AU) + schedule[5]; d = 0U + a + (((0U + (d)) << (12)) | ((d) >> (32 - (12))));
    c = 0U + c + (b ^ (d & (a ^ b))) + (0xA8304613U) + schedule[6]; c = 0U + d + (((0U + (c)) << (17)) | ((c) >> (32 - (17))));
    b = 0U + b + (a ^ (c & (d ^ a))) + (0xFD469501U) + schedule[7]; b = 0U + c + (((0U + (b)) << (22)) | ((b) >> (32 - (22))));
    a = 0U + a + (d ^ (b & (c ^ d))) + (0x698098D8U) + schedule[8]; a = 0U + b + (((0U + (a)) << (7)) | ((a) >> (32 - (7))));
    d = 0U + d + (c ^ (a & (b ^ c))) + (0x8B44F7AFU) + schedule[9]; d = 0U + a + (((0U + (d)) << (12)) | ((d) >> (32 - (12))));
    c = 0U + c + (b ^ (d & (a ^ b))) + (0xFFFF5BB1U) + schedule[10]; c = 0U + d + (((0U + (c)) << (17)) | ((c) >> (32 - (17))));
    b = 0U + b + (a ^ (c & (d ^ a))) + (0x895CD7BEU) + schedule[11]; b = 0U + c + (((0U + (b)) << (22)) | ((b) >> (32 - (22))));
    a = 0U + a + (d ^ (b & (c ^ d))) + (0x6B901122U) + schedule[12]; a = 0U + b + (((0U + (a)) << (7)) | ((a) >> (32 - (7))));
    d = 0U + d + (c ^ (a & (b ^ c))) + (0xFD987193U) + schedule[13]; d = 0U + a + (((0U + (d)) << (12)) | ((d) >> (32 - (12))));
    c = 0U + c + (b ^ (d & (a ^ b))) + (0xA679438EU) + schedule[14]; c = 0U + d + (((0U + (c)) << (17)) | ((c) >> (32 - (17))));
    b = 0U + b + (a ^ (c & (d ^ a))) + (0x49B40821U) + schedule[15]; b = 0U + c + (((0U + (b)) << (22)) | ((b) >> (32 - (22))));
    a = 0U + a + (c ^ (d & (b ^ c))) + (0xF61E2562U) + schedule[1]; a = 0U + b + (((0U + (a)) << (5)) | ((a) >> (32 - (5))));
    d = 0U + d + (b ^ (c & (a ^ b))) + (0xC040B340U) + schedule[6]; d = 0U + a + (((0U + (d)) << (9)) | ((d) >> (32 - (9))));
    c = 0U + c + (a ^ (b & (d ^ a))) + (0x265E5A51U) + schedule[11]; c = 0U + d + (((0U + (c)) << (14)) | ((c) >> (32 - (14))));
    b = 0U + b + (d ^ (a & (c ^ d))) + (0xE9B6C7AAU) + schedule[0]; b = 0U + c + (((0U + (b)) << (20)) | ((b) >> (32 - (20))));
    a = 0U + a + (c ^ (d & (b ^ c))) + (0xD62F105DU) + schedule[5]; a = 0U + b + (((0U + (a)) << (5)) | ((a) >> (32 - (5))));
    d = 0U + d + (b ^ (c & (a ^ b))) + (0x02441453U) + schedule[10]; d = 0U + a + (((0U + (d)) << (9)) | ((d) >> (32 - (9))));
    c = 0U + c + (a ^ (b & (d ^ a))) + (0xD8A1E681U) + schedule[15]; c = 0U + d + (((0U + (c)) << (14)) | ((c) >> (32 - (14))));
    b = 0U + b + (d ^ (a & (c ^ d))) + (0xE7D3FBC8U) + schedule[4]; b = 0U + c + (((0U + (b)) << (20)) | ((b) >> (32 - (20))));
    a = 0U + a + (c ^ (d & (b ^ c))) + (0x21E1CDE6U) + schedule[9]; a = 0U + b + (((0U + (a)) << (5)) | ((a) >> (32 - (5))));
    d = 0U + d + (b ^ (c & (a ^ b))) + (0xC33707D6U) + schedule[14]; d = 0U + a + (((0U + (d)) << (9)) | ((d) >> (32 - (9))));
    c = 0U + c + (a ^ (b & (d ^ a))) + (0xF4D50D87U) + schedule[3]; c = 0U + d + (((0U + (c)) << (14)) | ((c) >> (32 - (14))));
    b = 0U + b + (d ^ (a & (c ^ d))) + (0x455A14EDU) + schedule[8]; b = 0U + c + (((0U + (b)) << (20)) | ((b) >> (32 - (20))));
    a = 0U + a + (c ^ (d & (b ^ c))) + (0xA9E3E905U) + schedule[13]; a = 0U + b + (((0U + (a)) << (5)) | ((a) >> (32 - (5))));
    d = 0U + d + (b ^ (c & (a ^ b))) + (0xFCEFA3F8U) + schedule[2]; d = 0U + a + (((0U + (d)) << (9)) | ((d) >> (32 - (9))));
    c = 0U + c + (a ^ (b & (d ^ a))) + (0x676F02D9U) + schedule[7]; c = 0U + d + (((0U + (c)) << (14)) | ((c) >> (32 - (14))));
    b = 0U + b + (d ^ (a & (c ^ d))) + (0x8D2A4C8AU) + schedule[12]; b = 0U + c + (((0U + (b)) << (20)) | ((b) >> (32 - (20))));
    a = 0U + a + (b ^ c ^ d) + (0xFFFA3942U) + schedule[5]; a = 0U + b + (((0U + (a)) << (4)) | ((a) >> (32 - (4))));
    d = 0U + d + (a ^ b ^ c) + (0x8771F681U) + schedule[8]; d = 0U + a + (((0U + (d)) << (11)) | ((d) >> (32 - (11))));
    c = 0U + c + (d ^ a ^ b) + (0x6D9D6122U) + schedule[11]; c = 0U + d + (((0U + (c)) << (16)) | ((c) >> (32 - (16))));
    b = 0U + b + (c ^ d ^ a) + (0xFDE5380CU) + schedule[14]; b = 0U + c + (((0U + (b)) << (23)) | ((b) >> (32 - (23))));
    a = 0U + a + (b ^ c ^ d) + (0xA4BEEA44U) + schedule[1]; a = 0U + b + (((0U + (a)) << (4)) | ((a) >> (32 - (4))));
    d = 0U + d + (a ^ b ^ c) + (0x4BDECFA9U) + schedule[4]; d = 0U + a + (((0U + (d)) << (11)) | ((d) >> (32 - (11))));
    c = 0U + c + (d ^ a ^ b) + (0xF6BB4B60U) + schedule[7]; c = 0U + d + (((0U + (c)) << (16)) | ((c) >> (32 - (16))));
    b = 0U + b + (c ^ d ^ a) + (0xBEBFBC70U) + schedule[10]; b = 0U + c + (((0U + (b)) << (23)) | ((b) >> (32 - (23))));
    a = 0U + a + (b ^ c ^ d) + (0x289B7EC6U) + schedule[13]; a = 0U + b + (((0U + (a)) << (4)) | ((a) >> (32 - (4))));
    d = 0U + d + (a ^ b ^ c) + (0xEAA127FAU) + schedule[0]; d = 0U + a + (((0U + (d)) << (11)) | ((d) >> (32 - (11))));
    c = 0U + c + (d ^ a ^ b) + (0xD4EF3085U) + schedule[3]; c = 0U + d + (((0U + (c)) << (16)) | ((c) >> (32 - (16))));
    b = 0U + b + (c ^ d ^ a) + (0x04881D05U) + schedule[6]; b = 0U + c + (((0U + (b)) << (23)) | ((b) >> (32 - (23))));
    a = 0U + a + (b ^ c ^ d) + (0xD9D4D039U) + schedule[9]; a = 0U + b + (((0U + (a)) << (4)) | ((a) >> (32 - (4))));
    d = 0U + d + (a ^ b ^ c) + (0xE6DB99E5U) + schedule[12]; d = 0U + a + (((0U + (d)) << (11)) | ((d) >> (32 - (11))));
    c = 0U + c + (d ^ a ^ b) + (0x1FA27CF8U) + schedule[15]; c = 0U + d + (((0U + (c)) << (16)) | ((c) >> (32 - (16))));
    b = 0U + b + (c ^ d ^ a) + (0xC4AC5665U) + schedule[2]; b = 0U + c + (((0U + (b)) << (23)) | ((b) >> (32 - (23))));
    a = 0U + a + (c ^ (b | ~d)) + (0xF4292244U) + schedule[0]; a = 0U + b + (((0U + (a)) << (6)) | ((a) >> (32 - (6))));
    d = 0U + d + (b ^ (a | ~c)) + (0x432AFF97U) + schedule[7]; d = 0U + a + (((0U + (d)) << (10)) | ((d) >> (32 - (10))));
    c = 0U + c + (a ^ (d | ~b)) + (0xAB9423A7U) + schedule[14]; c = 0U + d + (((0U + (c)) << (15)) | ((c) >> (32 - (15))));
    b = 0U + b + (d ^ (c | ~a)) + (0xFC93A039U) + schedule[5]; b = 0U + c + (((0U + (b)) << (21)) | ((b) >> (32 - (21))));
    a = 0U + a + (c ^ (b | ~d)) + (0x655B59C3U) + schedule[12]; a = 0U + b + (((0U + (a)) << (6)) | ((a) >> (32 - (6))));
    d = 0U + d + (b ^ (a | ~c)) + (0x8F0CCC92U) + schedule[3]; d = 0U + a + (((0U + (d)) << (10)) | ((d) >> (32 - (10))));
    c = 0U + c + (a ^ (d | ~b)) + (0xFFEFF47DU) + schedule[10]; c = 0U + d + (((0U + (c)) << (15)) | ((c) >> (32 - (15))));
    b = 0U + b + (d ^ (c | ~a)) + (0x85845DD1U) + schedule[1]; b = 0U + c + (((0U + (b)) << (21)) | ((b) >> (32 - (21))));
    a = 0U + a + (c ^ (b | ~d)) + (0x6FA87E4FU) + schedule[8]; a = 0U + b + (((0U + (a)) << (6)) | ((a) >> (32 - (6))));
    d = 0U + d + (b ^ (a | ~c)) + (0xFE2CE6E0U) + schedule[15]; d = 0U + a + (((0U + (d)) << (10)) | ((d) >> (32 - (10))));
    c = 0U + c + (a ^ (d | ~b)) + (0xA3014314U) + schedule[6]; c = 0U + d + (((0U + (c)) << (15)) | ((c) >> (32 - (15))));
    b = 0U + b + (d ^ (c | ~a)) + (0x4E0811A1U) + schedule[13]; b = 0U + c + (((0U + (b)) << (21)) | ((b) >> (32 - (21))));
    a = 0U + a + (c ^ (b | ~d)) + (0xF7537E82U) + schedule[4]; a = 0U + b + (((0U + (a)) << (6)) | ((a) >> (32 - (6))));
    d = 0U + d + (b ^ (a | ~c)) + (0xBD3AF235U) + schedule[11]; d = 0U + a + (((0U + (d)) << (10)) | ((d) >> (32 - (10))));
    c = 0U + c + (a ^ (d | ~b)) + (0x2AD7D2BBU) + schedule[2]; c = 0U + d + (((0U + (c)) << (15)) | ((c) >> (32 - (15))));
    b = 0U + b + (d ^ (c | ~a)) + (0xEB86D391U) + schedule[9]; b = 0U + c + (((0U + (b)) << (21)) | ((b) >> (32 - (21))));

    state[0] = 0U + state[0] + a;
    state[1] = 0U + state[1] + b;
    state[2] = 0U + state[2] + c;
    state[3] = 0U + state[3] + d;
}

__kernel void compute(__global SortedDataHash* sdh, int type, int len, uchar4 param1)
{
	const size_t i = get_global_id (0);

	if (i < SLICE_LEN)
    {
        uint8_t data[BLOCK_LEN]= {0};
        uint32_t hash[STATE_LEN] = {0x67452301UL, 0xEFCDAB89UL, 0x98BADCFEUL, 0x10325476UL};

        if(type == DATA_TYPE_MOBILE)
        {
            // fill mobile digits
            data[MOBILE_LEN] = 0x80;
            data[BLOCK_LEN - LENGTH_SIZE] = 'X';
            data[0] = param1[0];
            data[1] = param1[1];
            data[2] = param1[2];
            size_t n = i;
            for(int j = 7; j >= 0; j--)
            {
                if(n == 0)
                {
                    data[3+j] = '0';
                }
                else
                {
                    data[3+j] = n % 10 + '0';
                    n /= 10;
                }
            }
        }

        md5_compress(hash, data);

        int index = binary_search(sdh, len, hash);
        if (index >= 0)
        {
            if(type == DATA_TYPE_MOBILE)
            {
                for(int j = 0; j < MOBILE_LEN; j++)
                {
                    sdh[index].data_hash.data[j] = data[j];
                }
            }
        }
    }
}
)";
