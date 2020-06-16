#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
	#include "OpenCL/opencl.h"
#else
	#include "CL/cl.h"
#endif

#include "mm.h"

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

// global opencl variables
cl_context context;
cl_program program;
cl_kernel kernel;
cl_mem aBuffer;
cl_mem bBuffer;
cl_device_id device_id;
cl_command_queue queue;

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

int is_equal(const MobileHash* a, const MobileHash* b)
{
    if (a->hash[0] == b->hash[0]
        && a->hash[1] == b->hash[1]
        && a->hash[2] == b->hash[2]
        && a->hash[3] == b->hash[3])
        return 1;

    return 0;
}

int is_lesser(const MobileHash* a, const MobileHash* b)
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

void _CheckCLError (cl_int error, int line)
{
	if (error != CL_SUCCESS) {
		printf("OpenCL call failed with error %d @%d\n", error, line);
		exit (1);
	}
}
#define CheckCLError(error) _CheckCLError(error, __LINE__);

int init_opencl()
{
	cl_uint platformIdCount = 0;
	clGetPlatformIDs (0, nullptr, &platformIdCount);

	if (platformIdCount == 0) {
        printf("No OpenCL platform found\n");
		return 1;
	}

    cl_platform_id platformIds[platformIdCount];
	clGetPlatformIDs (platformIdCount, platformIds, nullptr);

	cl_uint deviceIdCount = 0;
	clGetDeviceIDs (platformIds [0], CL_DEVICE_TYPE_ALL, 0, nullptr,
		&deviceIdCount);

	if (deviceIdCount == 0) {
        printf("No OpenCL devices found\n");
		return 1;
	}

	cl_device_id deviceIds [deviceIdCount];
	clGetDeviceIDs (platformIds [0], CL_DEVICE_TYPE_ALL, deviceIdCount,
		deviceIds, nullptr);

    device_id = deviceIds[1];

	const cl_context_properties contextProperties [] =
	{
		CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties> (platformIds [0]),
		0, 0
	};

	cl_int error = CL_SUCCESS;
	context = clCreateContext (contextProperties, deviceIdCount,
		deviceIds, nullptr, nullptr, &error);
	CheckCLError (error);

    size_t lengths [1] = { strlen(compute_cl) };
    const char* sources [1] = { compute_cl };
    program = clCreateProgramWithSource (context, 1, sources, lengths, &error);
	CheckCLError (error);
    // program = CreateProgram("kernels/compute.cl", context);

	CheckCLError (clBuildProgram (program, deviceIdCount, deviceIds, nullptr, nullptr, nullptr));

	kernel = clCreateKernel (program, "compute", &error);
	CheckCLError (error);

	queue = clCreateCommandQueue (context, device_id,
		0, &error);
	CheckCLError (error);

    return 0;
}
void release_opencl() {	
	clReleaseCommandQueue (queue);
	clReleaseMemObject (aBuffer);
	clReleaseMemObject (bBuffer);
	clReleaseKernel (kernel);
	clReleaseProgram (program);
	clReleaseContext (context);
}

int main(int argc, char* argv[])
{
    time_t start = time(NULL);

    // setup OpenCL
    if(init_opencl())
    {
        exit(1);
    }

    // process hash file
    char* s = NULL;
    if (argc < 2 || !(s = read_from_file(argv[1])))
    {
        printf("mobile md5 decoder [opencl], v1.0, by herman\n");
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
    size_t smh_len = decoder.dedup_len * sizeof(SortedMobileHash);
    cl_int error = CL_SUCCESS;
    aBuffer = clCreateBuffer (context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		smh_len,
		decoder.s_mobile_hash, &error);
	CheckCLError (error);
    bBuffer = clCreateBuffer (context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		10000 * 5,
		p_numbers, &error);
	CheckCLError (error);

    // release host memory
    free(p_numbers);

    // set kernel arg
    clSetKernelArg (kernel, 0, sizeof (cl_mem), &aBuffer);
    clSetKernelArg (kernel, 1, sizeof (cl_mem), &bBuffer);
    clSetKernelArg (kernel, 2, sizeof (int), &decoder.dedup_len);

    printf("0%% @%lds - 0/%zu\n", time(NULL) - start, decoder.dedup_len);
    // work on each prefix
    for (size_t i = 0; i < PREFIX_SIZE; i++)
    {
        uint8_t prefix0 = PREFIX_LIST[i] / 100 + '0';
        uint8_t prefix1 = (PREFIX_LIST[i] % 100) / 10 + '0';
        uint8_t prefix2 = PREFIX_LIST[i] % 10 + '0';

        // set kernel arg
        clSetKernelArg (kernel, 3, sizeof (uint8_t), &prefix0);
        clSetKernelArg (kernel, 4, sizeof (uint8_t), &prefix1);
        clSetKernelArg (kernel, 5, sizeof (uint8_t), &prefix2);
        // call opencl
        const size_t globalWorkSize [] = { SLICE_LEN, 0, 0 };
	    CheckCLError (clEnqueueNDRangeKernel (queue, kernel, 1,
            nullptr,
            globalWorkSize,
            nullptr,
            0, nullptr, nullptr));

        CheckCLError (clEnqueueReadBuffer (queue, aBuffer, CL_TRUE, 0,
            smh_len,
            decoder.s_mobile_hash,
            0, nullptr, nullptr));

        // compute << <blocks, threads >> > (d_smh, decoder.dedup_len, prefix[0], prefix[1], prefix[2], d_p_numbers);
        decoder.count = 0;
        for (size_t h = 0; h < decoder.dedup_len; h++)
        {
            if (decoder.s_mobile_hash[h].mobile_hash.mobile[0])
            {
                decoder.count++;
            }
        }
        printf("\033[1A%lu%% @%lds - %zu/%zu\n", (i+1)*100/ PREFIX_SIZE, time(NULL) - start, decoder.count, decoder.dedup_len);
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
    release_opencl();
    free_decoder(decoder);
}