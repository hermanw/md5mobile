#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// includes CUDA Runtime
#include <cuda_runtime.h>

#define SLICE_LEN 100000000
#define MOBILE_LEN 11
#define BLOCK_LEN 64 // In bytes
#define STATE_LEN 4  // In words
#define LENGTH_SIZE 8 // In bytes
#define HASH_LEN 32

typedef struct
{
    uint8_t mobile[BLOCK_LEN];
    uint32_t hash[STATE_LEN];
} MobileHash;

typedef struct
{
    int index;
    int index_dup;
    MobileHash mobile_hash;
} SortedMobileHash;

typedef char HashString[HASH_LEN];
typedef struct
{
    HashString* hash_string;
    size_t hash_len;
    MobileHash* mobile_hash;
    SortedMobileHash* s_mobile_hash;
    size_t dedup_len;
    size_t count;
} Decoder;

// constants
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
static const size_t PREFIX_SIZE = sizeof(PREFIX_LIST) / sizeof(PREFIX_LIST[0]);


__device__ void md5_compress(uint32_t state[4], const uint8_t block[64]) {
#define LOADSCHEDULE(i)  \
		schedule[i] = (uint32_t)block[i * 4 + 0] <<  0  \
		            | (uint32_t)block[i * 4 + 1] <<  8  \
		            | (uint32_t)block[i * 4 + 2] << 16  \
		            | (uint32_t)block[i * 4 + 3] << 24;

	uint32_t schedule[16];
	LOADSCHEDULE(0)
		LOADSCHEDULE(1)
		LOADSCHEDULE(2)
		LOADSCHEDULE(3)
		LOADSCHEDULE(4)
		LOADSCHEDULE(5)
		LOADSCHEDULE(6)
		LOADSCHEDULE(7)
		LOADSCHEDULE(8)
		LOADSCHEDULE(9)
		LOADSCHEDULE(10)
		LOADSCHEDULE(11)
		LOADSCHEDULE(12)
		LOADSCHEDULE(13)
		LOADSCHEDULE(14)
		LOADSCHEDULE(15)

#define ROTL32(x, n)  (((0U + (x)) << (n)) | ((x) >> (32 - (n))))  // Assumes that x is uint32_t and 0 < n < 32
#define ROUND0(a, b, c, d, k, s, t)  ROUND_TAIL(a, b, d ^ (b & (c ^ d)), k, s, t)
#define ROUND1(a, b, c, d, k, s, t)  ROUND_TAIL(a, b, c ^ (d & (b ^ c)), k, s, t)
#define ROUND2(a, b, c, d, k, s, t)  ROUND_TAIL(a, b, b ^ c ^ d        , k, s, t)
#define ROUND3(a, b, c, d, k, s, t)  ROUND_TAIL(a, b, c ^ (b | ~d)     , k, s, t)
#define ROUND_TAIL(a, b, expr, k, s, t)    \
		a = 0U + a + (expr) + UINT32_C(t) + schedule[k];  \
		a = 0U + b + ROTL32(a, s);

		uint32_t a = state[0];
	uint32_t b = state[1];
	uint32_t c = state[2];
	uint32_t d = state[3];

	ROUND0(a, b, c, d, 0, 7, 0xD76AA478)
		ROUND0(d, a, b, c, 1, 12, 0xE8C7B756)
		ROUND0(c, d, a, b, 2, 17, 0x242070DB)
		ROUND0(b, c, d, a, 3, 22, 0xC1BDCEEE)
		ROUND0(a, b, c, d, 4, 7, 0xF57C0FAF)
		ROUND0(d, a, b, c, 5, 12, 0x4787C62A)
		ROUND0(c, d, a, b, 6, 17, 0xA8304613)
		ROUND0(b, c, d, a, 7, 22, 0xFD469501)
		ROUND0(a, b, c, d, 8, 7, 0x698098D8)
		ROUND0(d, a, b, c, 9, 12, 0x8B44F7AF)
		ROUND0(c, d, a, b, 10, 17, 0xFFFF5BB1)
		ROUND0(b, c, d, a, 11, 22, 0x895CD7BE)
		ROUND0(a, b, c, d, 12, 7, 0x6B901122)
		ROUND0(d, a, b, c, 13, 12, 0xFD987193)
		ROUND0(c, d, a, b, 14, 17, 0xA679438E)
		ROUND0(b, c, d, a, 15, 22, 0x49B40821)
		ROUND1(a, b, c, d, 1, 5, 0xF61E2562)
		ROUND1(d, a, b, c, 6, 9, 0xC040B340)
		ROUND1(c, d, a, b, 11, 14, 0x265E5A51)
		ROUND1(b, c, d, a, 0, 20, 0xE9B6C7AA)
		ROUND1(a, b, c, d, 5, 5, 0xD62F105D)
		ROUND1(d, a, b, c, 10, 9, 0x02441453)
		ROUND1(c, d, a, b, 15, 14, 0xD8A1E681)
		ROUND1(b, c, d, a, 4, 20, 0xE7D3FBC8)
		ROUND1(a, b, c, d, 9, 5, 0x21E1CDE6)
		ROUND1(d, a, b, c, 14, 9, 0xC33707D6)
		ROUND1(c, d, a, b, 3, 14, 0xF4D50D87)
		ROUND1(b, c, d, a, 8, 20, 0x455A14ED)
		ROUND1(a, b, c, d, 13, 5, 0xA9E3E905)
		ROUND1(d, a, b, c, 2, 9, 0xFCEFA3F8)
		ROUND1(c, d, a, b, 7, 14, 0x676F02D9)
		ROUND1(b, c, d, a, 12, 20, 0x8D2A4C8A)
		ROUND2(a, b, c, d, 5, 4, 0xFFFA3942)
		ROUND2(d, a, b, c, 8, 11, 0x8771F681)
		ROUND2(c, d, a, b, 11, 16, 0x6D9D6122)
		ROUND2(b, c, d, a, 14, 23, 0xFDE5380C)
		ROUND2(a, b, c, d, 1, 4, 0xA4BEEA44)
		ROUND2(d, a, b, c, 4, 11, 0x4BDECFA9)
		ROUND2(c, d, a, b, 7, 16, 0xF6BB4B60)
		ROUND2(b, c, d, a, 10, 23, 0xBEBFBC70)
		ROUND2(a, b, c, d, 13, 4, 0x289B7EC6)
		ROUND2(d, a, b, c, 0, 11, 0xEAA127FA)
		ROUND2(c, d, a, b, 3, 16, 0xD4EF3085)
		ROUND2(b, c, d, a, 6, 23, 0x04881D05)
		ROUND2(a, b, c, d, 9, 4, 0xD9D4D039)
		ROUND2(d, a, b, c, 12, 11, 0xE6DB99E5)
		ROUND2(c, d, a, b, 15, 16, 0x1FA27CF8)
		ROUND2(b, c, d, a, 2, 23, 0xC4AC5665)
		ROUND3(a, b, c, d, 0, 6, 0xF4292244)
		ROUND3(d, a, b, c, 7, 10, 0x432AFF97)
		ROUND3(c, d, a, b, 14, 15, 0xAB9423A7)
		ROUND3(b, c, d, a, 5, 21, 0xFC93A039)
		ROUND3(a, b, c, d, 12, 6, 0x655B59C3)
		ROUND3(d, a, b, c, 3, 10, 0x8F0CCC92)
		ROUND3(c, d, a, b, 10, 15, 0xFFEFF47D)
		ROUND3(b, c, d, a, 1, 21, 0x85845DD1)
		ROUND3(a, b, c, d, 8, 6, 0x6FA87E4F)
		ROUND3(d, a, b, c, 15, 10, 0xFE2CE6E0)
		ROUND3(c, d, a, b, 6, 15, 0xA3014314)
		ROUND3(b, c, d, a, 13, 21, 0x4E0811A1)
		ROUND3(a, b, c, d, 4, 6, 0xF7537E82)
		ROUND3(d, a, b, c, 11, 10, 0xBD3AF235)
		ROUND3(c, d, a, b, 2, 15, 0x2AD7D2BB)
		ROUND3(b, c, d, a, 9, 21, 0xEB86D391)

		state[0] = 0U + state[0] + a;
	state[1] = 0U + state[1] + b;
	state[2] = 0U + state[2] + c;
	state[3] = 0U + state[3] + d;
}

__host__ __device__ int is_equal(const MobileHash* a, const MobileHash* b)
{
    if (a->hash[0] == b->hash[0]
        && a->hash[1] == b->hash[1]
        && a->hash[2] == b->hash[2]
        && a->hash[3] == b->hash[3])
        return 1;

    return 0;
}

__host__ __device__ int is_lesser(const MobileHash* a, const MobileHash* b)
{
    if (a->hash[0] < b->hash[0])
    {
        return 1;
    }
    else if (a->hash[0] == b->hash[0]) {
        if (a->hash[1] < b->hash[1])
        {
            return 1;
        }
        else if (a->hash[1] == b->hash[1]) {
            if (a->hash[2] < b->hash[2])
            {
                return 1;
            }
            else if (a->hash[2] == b->hash[2]) {
                if (a->hash[3] < b->hash[3])
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

__device__ int binary_search(SortedMobileHash* array, int len, const MobileHash* key)
{
    int low = 0, high = len - 1, mid;
    while (low <= high)
    {
        mid = (low + high) / 2;
        if (is_equal(&array[mid].mobile_hash, key))
        {
            return mid;
        }
        else if (is_lesser(&array[mid].mobile_hash, key))
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

__global__ void compute(SortedMobileHash* smh, int dedup_len, uint8_t prefix0, uint8_t prefix1, uint8_t prefix2, char* p_numbers)
{
    uint32_t i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < SLICE_LEN)
    {
        MobileHash mh;
        memset(mh.mobile, 0, BLOCK_LEN);
        mh.mobile[0] = prefix0;
        mh.mobile[1] = prefix1;
        mh.mobile[2] = prefix2;
        memcpy(mh.mobile + 3, p_numbers + (i/10000) * 5, 4);
        memcpy(mh.mobile + 7, p_numbers + (i % 10000) * 5, 4);
        mh.mobile[MOBILE_LEN] = 0x80;
        mh.mobile[BLOCK_LEN - LENGTH_SIZE] = 'X';

        mh.hash[0] = UINT32_C(0x67452301);
        mh.hash[1] = UINT32_C(0xEFCDAB89);
        mh.hash[2] = UINT32_C(0x98BADCFE);
        mh.hash[3] = UINT32_C(0x10325476);
        md5_compress(mh.hash, mh.mobile);

        int index = binary_search(smh, dedup_len, &mh);
        if (index >= 0)
        {
            smh[index].mobile_hash = mh;
        }
    }
}


char* read_from_file(char* filename)
{
    FILE* f = fopen(filename, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buffer = (char*)malloc(len + 1);
        len = fread(buffer, 1, len, f);
        fclose(f);
        buffer[len] = 0;
        return buffer;
    }
    return 0;
}

void write_to_file(char* filename, Decoder* decoder)
{
    FILE* f = fopen(filename, "w");
    for (int h = 0; h < decoder->hash_len; h++)
    {
        for (size_t i = 0; i < HASH_LEN; i++)
        {
            fputc(decoder->hash_string[h][i], f);
        }
        fputc(',', f);
        for (size_t i = 0; i < MOBILE_LEN; i++)
        {
            fputc(decoder->mobile_hash[h].mobile[i], f);
        }
        fputc('\n', f);
    }
    fclose(f);
}

void setupCuda()
{
    cudaDeviceProp deviceProps;
    if (cudaGetDeviceProperties(&deviceProps, 0))
    {
        printf("no CUDA devices\n");
        exit(1);
    }
    printf("use CUDA device [%s]\n", deviceProps.name);
    cudaSetDevice(0);
}

void quick_sort(SortedMobileHash* array, int from, int to)
{
    if (from >= to)return;
    SortedMobileHash temp;
    int i = from, j;
    for (j = from + 1; j <= to; j++)
    {
        if (is_lesser(&array[j].mobile_hash, &array[from].mobile_hash))
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
    quick_sort(array, from, i - 1);
    quick_sort(array, i + 1, to);
}

inline char hexToNibble(char n)
{
    return n - (n <= '9' ? '0' : ('a' - 10));
}
void hex_to_bytes(uint8_t* to, char* from, int len)
{
    for (int i = 0; i < len / 2; i++)
    {
        to[i] = (hexToNibble(from[i * 2]) << 4) + hexToNibble(from[i * 2 + 1]);
    }
}

void print_mobile_hash(MobileHash* mh) {
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
void print_sorted_mobile_hash(SortedMobileHash* smh) {
    printf("%d,%d,", smh->index, smh->index_dup);
    print_mobile_hash(&smh->mobile_hash);
}

size_t validate_hash_string(const char* s)
{
    int valid_char_num = 0;
    int count = 0;
    while (*s)
    {
        if (*s == ',')
        {
            if (valid_char_num == HASH_LEN)
            {
                count++;
            }
            valid_char_num = 0;
        }
        else if ((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9'))
        {
            valid_char_num++;
        }
        s++;
    }
    if (valid_char_num == HASH_LEN)
    {
        count++;
    }

    return count;
}

void parse_hash_strings(Decoder& decoder, const char* s)
{
    SortedMobileHash* p_smh = decoder.s_mobile_hash;
    HashString* p_hs = decoder.hash_string;

    int valid_char_num = 0;
    int count = 0;
    char hash_string[HASH_LEN];
    while (*s)
    {
        if (*s == ',')
        {
            if (valid_char_num == HASH_LEN)
            {
                memcpy(p_hs, hash_string, HASH_LEN);
                hex_to_bytes((uint8_t*)p_smh->mobile_hash.hash, hash_string, HASH_LEN);
                p_smh->index = count;
                count++;
                p_smh++;
                p_hs++;
            }
            valid_char_num = 0;
        }
        else if ((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9'))
        {
            hash_string[valid_char_num] = *s;
            valid_char_num++;
        }
        s++;
    }
    if (valid_char_num == HASH_LEN)
    {
        memcpy(p_hs, hash_string, HASH_LEN);
        hex_to_bytes((uint8_t*)p_smh->mobile_hash.hash, hash_string, HASH_LEN);
        p_smh->index = count;
    }
}

void dedup_sorted_mobile_hash(Decoder& decoder)
{
    decoder.dedup_len = decoder.hash_len;
    for (size_t i = 1; i < decoder.dedup_len; i++)
    {
        if (is_equal(&decoder.s_mobile_hash[i].mobile_hash, &decoder.s_mobile_hash[i - 1].mobile_hash))
        {
            SortedMobileHash temp = decoder.s_mobile_hash[i];
            temp.index_dup = decoder.s_mobile_hash[i - 1].index;
            for (size_t j = i; j < decoder.hash_len - 1; j++)
            {
                decoder.s_mobile_hash[j] = decoder.s_mobile_hash[j + 1];
            }
            decoder.s_mobile_hash[decoder.hash_len - 1] = temp;
            decoder.dedup_len--;
            i--;
        }
    }
}


void resort_mobile_hash(Decoder& decoder)
{
    for (int i = 0; i < decoder.hash_len; i++)
    {
        if (i < decoder.dedup_len)
        {
            int index = decoder.s_mobile_hash[i].index;
            decoder.mobile_hash[index] = decoder.s_mobile_hash[i].mobile_hash;
        }
        else
        {
            int index = decoder.s_mobile_hash[i].index;
            int index_dup = decoder.s_mobile_hash[i].index_dup;
            decoder.mobile_hash[index] = decoder.mobile_hash[index_dup];
        }
    }
}

void init_decoder(Decoder& decoder,const char* s)
{
    decoder.hash_len = validate_hash_string(s);
    decoder.hash_string = (HashString*)calloc(decoder.hash_len, sizeof(HashString));
    decoder.mobile_hash = (MobileHash*)calloc(decoder.hash_len, sizeof(MobileHash));
    decoder.s_mobile_hash = (SortedMobileHash*)calloc(decoder.hash_len, sizeof(SortedMobileHash));
    decoder.count = 0;
    parse_hash_strings(decoder, s);
    quick_sort(decoder.s_mobile_hash, 0, decoder.hash_len - 1);
    dedup_sorted_mobile_hash(decoder);
    // for (size_t i = 0; i < decoder.hash_len; i++)
    // {
    //     printf("%zu,", i);
    //     print_sorted_mobile_hash(decoder.s_mobile_hash+i);
    // }
}
void free_decoder(Decoder& decoder)
{
    free(decoder.hash_string);
    decoder.hash_string = 0;
    free(decoder.mobile_hash);
    decoder.mobile_hash = 0;
    free(decoder.s_mobile_hash);
    decoder.s_mobile_hash = 0;
}

void check(cudaError_t e, int const line)
{
    if (e)
    {
        printf("cudaError(%d) @%d\n", e, line);
        exit(e);
    }
}

#define check_error(val) check(val, __LINE__)

int main(int argc, char* argv[])
{
    time_t start = time(NULL);

    // setup cuda
    setupCuda();

    // process hash file
    char* s = NULL;
    if (argc < 2 || !(s = read_from_file(argv[1])))
    {
        printf("mobile md5 decoder [cuda], v1.0, by herman\n");
        printf("usage: mm filename\n");
        return 1;
    }

    Decoder decoder;
    init_decoder(decoder, s);
    free(s);
    s = 0;
    printf("find %zu hashes\n", decoder.hash_len);
    printf("they have %zu duplicated, %zu unique ones\n", decoder.hash_len - decoder.dedup_len, decoder.dedup_len);

    // compute helper strings
    char* p_numbers = (char *)malloc(10000 * 5);
    for (size_t i = 0; i < 10000; i++)
    {
        sprintf(p_numbers+i*5, "%04d", i);
    }

    // allocate device memory
    SortedMobileHash* d_smh;
    size_t smh_len = decoder.dedup_len * sizeof(SortedMobileHash);
    check_error(cudaMalloc((void**)&d_smh, smh_len));
    check_error(cudaMemcpy(d_smh, decoder.s_mobile_hash, smh_len, cudaMemcpyHostToDevice));
    char* d_p_numbers;
    check_error(cudaMalloc((void**)&d_p_numbers, 10000 * 5));
    check_error(cudaMemcpy(d_p_numbers, p_numbers, 10000 * 5, cudaMemcpyHostToDevice));

    // release host memory
    free(p_numbers);

    // set kernel launch configuration
    int threads = 512;
    int blocks = (SLICE_LEN+511) / 512;

    printf("0%% @%lds - 0/%zu\n", time(NULL) - start, decoder.dedup_len);
    // work on each prefix
    for (size_t i = 0; i < PREFIX_SIZE; i++)
    {
        uint8_t prefix[3];
        prefix[0] = PREFIX_LIST[i] / 100 + '0';
        prefix[1] = (PREFIX_LIST[i] % 100) / 10 + '0';
        prefix[2] = PREFIX_LIST[i] % 10 + '0';
        compute << <blocks, threads >> > (d_smh, decoder.dedup_len, prefix[0], prefix[1], prefix[2], d_p_numbers);
        check_error(cudaDeviceSynchronize());
        check_error(cudaMemcpy(decoder.s_mobile_hash, d_smh, smh_len, cudaMemcpyDeviceToHost));
        decoder.count = 0;
        for (size_t h = 0; h < decoder.dedup_len; h++)
        {
            if (decoder.s_mobile_hash[h].mobile_hash.mobile[0])
            {
                decoder.count++;
            }
        }
        printf("\033[1A%d%% @%lds - %zu/%zu\n", (i+1)*100/ PREFIX_SIZE, time(NULL) - start, decoder.count, decoder.dedup_len);
        if (decoder.count == decoder.dedup_len)
        {
            break;
        }
    }

    // write reults
    printf("total %zu hashes are decoded\n", decoder.count);
    resort_mobile_hash(decoder);
    size_t fn_len = strlen(argv[1]) + 5;
    char* outfile = (char*)malloc(fn_len);
    strcpy(outfile, argv[1]);
    strcat(outfile, ".out");
    write_to_file(outfile, &decoder);
    printf("please find results in file: %s\n", outfile);
    free(outfile);

    // release resources
    cudaFree(d_smh);
    cudaFree(d_p_numbers);
    free_decoder(decoder);
}
