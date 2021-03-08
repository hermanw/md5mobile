#define BLOCK_LEN 64 // In bytes
#define LENGTH_SIZE 8 // In bytes

// macro from compiler
// #define DATA_LENGTH 18
// #define X_INDEX 0
// #define X_LENGTH 6
// #define X_TYPE 0
// #define Y_INDEX 6
// #define Y_LENGTH 3
// #define Y_TYPE 1
// #define Z_INDEX 17
// #define Z_LENGTH 1
// #define Z_TYPE 2

static void md5_compress(uint4* state, const uchar block[64])
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

    unsigned int a = state->s0;
    unsigned int b = state->s1;
    unsigned int c = state->s2;
    unsigned int d = state->s3;

    a = a + (d ^ (b & (c ^ d))) + (0xD76AA478U) + schedule[0]; a = b + ((((a)) << (7)) | ((a) >> (32 - (7))));
    d = d + (c ^ (a & (b ^ c))) + (0xE8C7B756U) + schedule[1]; d = a + ((((d)) << (12)) | ((d) >> (32 - (12))));
    c = c + (b ^ (d & (a ^ b))) + (0x242070DBU) + schedule[2]; c = d + ((((c)) << (17)) | ((c) >> (32 - (17))));
    b = b + (a ^ (c & (d ^ a))) + (0xC1BDCEEEU) + schedule[3]; b = c + ((((b)) << (22)) | ((b) >> (32 - (22))));
    a = a + (d ^ (b & (c ^ d))) + (0xF57C0FAFU) + schedule[4]; a = b + ((((a)) << (7)) | ((a) >> (32 - (7))));
    d = d + (c ^ (a & (b ^ c))) + (0x4787C62AU) + schedule[5]; d = a + ((((d)) << (12)) | ((d) >> (32 - (12))));
    c = c + (b ^ (d & (a ^ b))) + (0xA8304613U) + schedule[6]; c = d + ((((c)) << (17)) | ((c) >> (32 - (17))));
    b = b + (a ^ (c & (d ^ a))) + (0xFD469501U) + schedule[7]; b = c + ((((b)) << (22)) | ((b) >> (32 - (22))));
    a = a + (d ^ (b & (c ^ d))) + (0x698098D8U) + schedule[8]; a = b + ((((a)) << (7)) | ((a) >> (32 - (7))));
    d = d + (c ^ (a & (b ^ c))) + (0x8B44F7AFU) + schedule[9]; d = a + ((((d)) << (12)) | ((d) >> (32 - (12))));
    c = c + (b ^ (d & (a ^ b))) + (0xFFFF5BB1U) + schedule[10]; c = d + ((((c)) << (17)) | ((c) >> (32 - (17))));
    b = b + (a ^ (c & (d ^ a))) + (0x895CD7BEU) + schedule[11]; b = c + ((((b)) << (22)) | ((b) >> (32 - (22))));
    a = a + (d ^ (b & (c ^ d))) + (0x6B901122U) + schedule[12]; a = b + ((((a)) << (7)) | ((a) >> (32 - (7))));
    d = d + (c ^ (a & (b ^ c))) + (0xFD987193U) + schedule[13]; d = a + ((((d)) << (12)) | ((d) >> (32 - (12))));
    c = c + (b ^ (d & (a ^ b))) + (0xA679438EU) + schedule[14]; c = d + ((((c)) << (17)) | ((c) >> (32 - (17))));
    b = b + (a ^ (c & (d ^ a))) + (0x49B40821U) + schedule[15]; b = c + ((((b)) << (22)) | ((b) >> (32 - (22))));
    a = a + (c ^ (d & (b ^ c))) + (0xF61E2562U) + schedule[1]; a = b + ((((a)) << (5)) | ((a) >> (32 - (5))));
    d = d + (b ^ (c & (a ^ b))) + (0xC040B340U) + schedule[6]; d = a + ((((d)) << (9)) | ((d) >> (32 - (9))));
    c = c + (a ^ (b & (d ^ a))) + (0x265E5A51U) + schedule[11]; c = d + ((((c)) << (14)) | ((c) >> (32 - (14))));
    b = b + (d ^ (a & (c ^ d))) + (0xE9B6C7AAU) + schedule[0]; b = c + ((((b)) << (20)) | ((b) >> (32 - (20))));
    a = a + (c ^ (d & (b ^ c))) + (0xD62F105DU) + schedule[5]; a = b + ((((a)) << (5)) | ((a) >> (32 - (5))));
    d = d + (b ^ (c & (a ^ b))) + (0x02441453U) + schedule[10]; d = a + ((((d)) << (9)) | ((d) >> (32 - (9))));
    c = c + (a ^ (b & (d ^ a))) + (0xD8A1E681U) + schedule[15]; c = d + ((((c)) << (14)) | ((c) >> (32 - (14))));
    b = b + (d ^ (a & (c ^ d))) + (0xE7D3FBC8U) + schedule[4]; b = c + ((((b)) << (20)) | ((b) >> (32 - (20))));
    a = a + (c ^ (d & (b ^ c))) + (0x21E1CDE6U) + schedule[9]; a = b + ((((a)) << (5)) | ((a) >> (32 - (5))));
    d = d + (b ^ (c & (a ^ b))) + (0xC33707D6U) + schedule[14]; d = a + ((((d)) << (9)) | ((d) >> (32 - (9))));
    c = c + (a ^ (b & (d ^ a))) + (0xF4D50D87U) + schedule[3]; c = d + ((((c)) << (14)) | ((c) >> (32 - (14))));
    b = b + (d ^ (a & (c ^ d))) + (0x455A14EDU) + schedule[8]; b = c + ((((b)) << (20)) | ((b) >> (32 - (20))));
    a = a + (c ^ (d & (b ^ c))) + (0xA9E3E905U) + schedule[13]; a = b + ((((a)) << (5)) | ((a) >> (32 - (5))));
    d = d + (b ^ (c & (a ^ b))) + (0xFCEFA3F8U) + schedule[2]; d = a + ((((d)) << (9)) | ((d) >> (32 - (9))));
    c = c + (a ^ (b & (d ^ a))) + (0x676F02D9U) + schedule[7]; c = d + ((((c)) << (14)) | ((c) >> (32 - (14))));
    b = b + (d ^ (a & (c ^ d))) + (0x8D2A4C8AU) + schedule[12]; b = c + ((((b)) << (20)) | ((b) >> (32 - (20))));
    a = a + (b ^ c ^ d) + (0xFFFA3942U) + schedule[5]; a = b + ((((a)) << (4)) | ((a) >> (32 - (4))));
    d = d + (a ^ b ^ c) + (0x8771F681U) + schedule[8]; d = a + ((((d)) << (11)) | ((d) >> (32 - (11))));
    c = c + (d ^ a ^ b) + (0x6D9D6122U) + schedule[11]; c = d + ((((c)) << (16)) | ((c) >> (32 - (16))));
    b = b + (c ^ d ^ a) + (0xFDE5380CU) + schedule[14]; b = c + ((((b)) << (23)) | ((b) >> (32 - (23))));
    a = a + (b ^ c ^ d) + (0xA4BEEA44U) + schedule[1]; a = b + ((((a)) << (4)) | ((a) >> (32 - (4))));
    d = d + (a ^ b ^ c) + (0x4BDECFA9U) + schedule[4]; d = a + ((((d)) << (11)) | ((d) >> (32 - (11))));
    c = c + (d ^ a ^ b) + (0xF6BB4B60U) + schedule[7]; c = d + ((((c)) << (16)) | ((c) >> (32 - (16))));
    b = b + (c ^ d ^ a) + (0xBEBFBC70U) + schedule[10]; b = c + ((((b)) << (23)) | ((b) >> (32 - (23))));
    a = a + (b ^ c ^ d) + (0x289B7EC6U) + schedule[13]; a = b + ((((a)) << (4)) | ((a) >> (32 - (4))));
    d = d + (a ^ b ^ c) + (0xEAA127FAU) + schedule[0]; d = a + ((((d)) << (11)) | ((d) >> (32 - (11))));
    c = c + (d ^ a ^ b) + (0xD4EF3085U) + schedule[3]; c = d + ((((c)) << (16)) | ((c) >> (32 - (16))));
    b = b + (c ^ d ^ a) + (0x04881D05U) + schedule[6]; b = c + ((((b)) << (23)) | ((b) >> (32 - (23))));
    a = a + (b ^ c ^ d) + (0xD9D4D039U) + schedule[9]; a = b + ((((a)) << (4)) | ((a) >> (32 - (4))));
    d = d + (a ^ b ^ c) + (0xE6DB99E5U) + schedule[12]; d = a + ((((d)) << (11)) | ((d) >> (32 - (11))));
    c = c + (d ^ a ^ b) + (0x1FA27CF8U) + schedule[15]; c = d + ((((c)) << (16)) | ((c) >> (32 - (16))));
    b = b + (c ^ d ^ a) + (0xC4AC5665U) + schedule[2]; b = c + ((((b)) << (23)) | ((b) >> (32 - (23))));
    a = a + (c ^ (b | ~d)) + (0xF4292244U) + schedule[0]; a = b + ((((a)) << (6)) | ((a) >> (32 - (6))));
    d = d + (b ^ (a | ~c)) + (0x432AFF97U) + schedule[7]; d = a + ((((d)) << (10)) | ((d) >> (32 - (10))));
    c = c + (a ^ (d | ~b)) + (0xAB9423A7U) + schedule[14]; c = d + ((((c)) << (15)) | ((c) >> (32 - (15))));
    b = b + (d ^ (c | ~a)) + (0xFC93A039U) + schedule[5]; b = c + ((((b)) << (21)) | ((b) >> (32 - (21))));
    a = a + (c ^ (b | ~d)) + (0x655B59C3U) + schedule[12]; a = b + ((((a)) << (6)) | ((a) >> (32 - (6))));
    d = d + (b ^ (a | ~c)) + (0x8F0CCC92U) + schedule[3]; d = a + ((((d)) << (10)) | ((d) >> (32 - (10))));
    c = c + (a ^ (d | ~b)) + (0xFFEFF47DU) + schedule[10]; c = d + ((((c)) << (15)) | ((c) >> (32 - (15))));
    b = b + (d ^ (c | ~a)) + (0x85845DD1U) + schedule[1]; b = c + ((((b)) << (21)) | ((b) >> (32 - (21))));
    a = a + (c ^ (b | ~d)) + (0x6FA87E4FU) + schedule[8]; a = b + ((((a)) << (6)) | ((a) >> (32 - (6))));
    d = d + (b ^ (a | ~c)) + (0xFE2CE6E0U) + schedule[15]; d = a + ((((d)) << (10)) | ((d) >> (32 - (10))));
    c = c + (a ^ (d | ~b)) + (0xA3014314U) + schedule[6]; c = d + ((((c)) << (15)) | ((c) >> (32 - (15))));
    b = b + (d ^ (c | ~a)) + (0x4E0811A1U) + schedule[13]; b = c + ((((b)) << (21)) | ((b) >> (32 - (21))));
    a = a + (c ^ (b | ~d)) + (0xF7537E82U) + schedule[4]; a = b + ((((a)) << (6)) | ((a) >> (32 - (6))));
    d = d + (b ^ (a | ~c)) + (0xBD3AF235U) + schedule[11]; d = a + ((((d)) << (10)) | ((d) >> (32 - (10))));
    c = c + (a ^ (d | ~b)) + (0x2AD7D2BBU) + schedule[2]; c = d + ((((c)) << (15)) | ((c) >> (32 - (15))));
    b = b + (d ^ (c | ~a)) + (0xEB86D391U) + schedule[9]; b = c + ((((b)) << (21)) | ((b) >> (32 - (21))));

    state->s0 += a;
    state->s1 += b;
    state->s2 += c;
    state->s3 += d;
}

__kernel void compute(
    __global int* p_count,
    __global uchar* p_data,
    __global const ulong2* p_hash,
    __constant uchar* p_number,
    __constant uchar* p_helper,
    __constant uchar* input,
    int hash_len)
{
    if(*p_count >= hash_len) return;

    // fill data
    uchar data[BLOCK_LEN]= {0};
    data[DATA_LENGTH] = 0x80;
    data[BLOCK_LEN - LENGTH_SIZE] = (uchar)(DATA_LENGTH << 3);
    for (int i = 0; i < DATA_LENGTH; i++) {
        data[i] = input[i];
    }

#ifdef X_TYPE
#if X_TYPE == 0
    uint x_offset = get_global_id(0) * X_LENGTH;
    for (uint i = 0; i < X_LENGTH; i++)
    {
        data[X_INDEX + i] = p_helper[x_offset + i];
    }
#elif X_TYPE == 1
    uint x_offset = (get_global_id(0) << 2) + 4 - X_LENGTH;
    for (uint i = 0; i < X_LENGTH; i++)
    {
        data[X_INDEX + i] = p_number[x_offset + i];
    }
#endif
#endif

#ifdef Y_TYPE
#if Y_TYPE == 0
    uint y_offset = get_global_id(1) * Y_LENGTH;
    for (uint i = 0; i < Y_LENGTH; i++)
    {
        data[Y_INDEX + i] = p_helper[y_offset + i];
    }
#elif Y_TYPE == 1
    uint y_offset = (get_global_id(1) << 2) + 4 - Y_LENGTH;
    for (uint i = 0; i < Y_LENGTH; i++)
    {
        data[Y_INDEX + i] = p_number[y_offset + i];
    }
#endif
#endif

#ifdef Z_TYPE
#if Z_TYPE == 0
    uint z_offset = get_global_id(2) * Z_LENGTH;
    for (uint i = 0; i < Z_LENGTH; i++)
    {
        data[Z_INDEX + i] = p_helper[z_offset + i];
    }
#elif Z_TYPE == 1
    uint z_offset = (get_global_id(2) << 2) + 4 - Z_LENGTH;
    for (uint i = 0; i < Z_LENGTH; i++)
    {
        data[Z_INDEX + i] = p_number[z_offset + i];
    }
#elif Z_TYPE == 2
    const uchar COE[17] = {7,9,10,5,8,4,2,1,6,3,7,9,10,5,8,4,2};
    const uchar C_SUM[11] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'X'};
    uint sum = 0;
    for (uint i = 0; i < 17; i++)
    {
        sum += (data[i] - '0') * COE[i];
    }
    uint r = sum % 11;
    r = 12 - r;
    r = select(r, r-11, r>10);
    data[Z_INDEX] =C_SUM[r];
#endif
#endif

    // hash
    uint4 hash = (uint4)(0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476);

    md5_compress(&hash, data);

    ulong2 bhash = as_ulong2(hash);
    int low = 0;
    int high = hash_len - 1;
    do
    {
        int mid = (low + high) / 2;
        ulong2 ahash = p_hash[mid];
        if(ahash.s0 < bhash.s0)
        {
            low = mid + 1;
        }
        else if (ahash.s0 > bhash.s0)
        {
            high = mid - 1;
        }
        else
        {
            if(ahash.s1 < bhash.s1) {low = mid + 1;}
            else if (ahash.s1 > bhash.s1) {high = mid - 1;}
            else
            {
                atomic_inc(p_count);
                int offset = mid * DATA_LENGTH;
                for (int j = 0; j < DATA_LENGTH; j++)
                {
                    p_data[offset + j] = data[j];
                }
                break;
            }
        }
    } while (low <= high);
}
