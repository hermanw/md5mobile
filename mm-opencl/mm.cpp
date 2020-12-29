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
    uint8_t data[BLOCK_LEN];
    uint32_t hash[STATE_LEN];
} DataHash;

typedef struct
{
    int index;
    int index_dup;
    DataHash data_hash;
} SortedDataHash;

#define DATA_TYPE_MOBILE 0
#define DATA_TYPE_CNID 1

typedef char HashString[HASH_LEN];
typedef struct
{
    HashString* hash_string;
    size_t hash_len;
    DataHash* data_hash;
    SortedDataHash* s_data_hash;
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
            fputc(decoder->data_hash[h].data[i], f);
        }
        fputc('\n', f);
    }
    fclose(f);
}

int compare_hash(const uint32_t* a, const uint32_t* b)
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

void quick_sort(SortedDataHash* array, int from, int to)
{
    if (from >= to)return;
    SortedDataHash temp;
    int i = from, j;
    for (j = from + 1; j <= to; j++)
    {
        if (compare_hash(array[j].data_hash.hash, array[from].data_hash.hash) < 0)
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

void print_data_hash(DataHash* dh) {
    for (size_t i = 0; i < STATE_LEN; i++)
    {
        printf("%x-", dh->hash[i]);
    }
    printf(",");
    for (size_t i = 0; i < MOBILE_LEN; i++)
    {
        printf("%c", dh->data[i]);
    }
    printf("\n");
}
void print_sorted_data_hash(SortedDataHash* sdh) {
    printf("%d,%d,", sdh->index, sdh->index_dup);
    print_data_hash(&sdh->data_hash);
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
    SortedDataHash* p_sdh = decoder.s_data_hash;
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
                hex_to_bytes((uint8_t*)p_sdh->data_hash.hash, hash_string, HASH_LEN);
                p_sdh->index = count;
                count++;
                p_sdh++;
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
        hex_to_bytes((uint8_t*)p_sdh->data_hash.hash, hash_string, HASH_LEN);
        p_sdh->index = count;
    }
}

void dedup_sorted_data_hash(Decoder& decoder)
{
    decoder.dedup_len = decoder.hash_len;
    for (size_t i = 1; i < decoder.dedup_len; i++)
    {
        if (compare_hash(decoder.s_data_hash[i].data_hash.hash, decoder.s_data_hash[i - 1].data_hash.hash) == 0)
        {
            SortedDataHash temp = decoder.s_data_hash[i];
            temp.index_dup = decoder.s_data_hash[i - 1].index;
            for (size_t j = i; j < decoder.hash_len - 1; j++)
            {
                decoder.s_data_hash[j] = decoder.s_data_hash[j + 1];
            }
            decoder.s_data_hash[decoder.hash_len - 1] = temp;
            decoder.dedup_len--;
            i--;
        }
    }
}


void resort_data_hash(Decoder& decoder)
{
    for (int i = 0; i < decoder.hash_len; i++)
    {
        if (i < decoder.dedup_len)
        {
            int index = decoder.s_data_hash[i].index;
            decoder.data_hash[index] = decoder.s_data_hash[i].data_hash;
        }
        else
        {
            int index = decoder.s_data_hash[i].index;
            int index_dup = decoder.s_data_hash[i].index_dup;
            decoder.data_hash[index] = decoder.data_hash[index_dup];
        }
    }
}

void init_decoder(Decoder& decoder,const char* s)
{
    decoder.hash_len = validate_hash_string(s);
    decoder.hash_string = (HashString*)calloc(decoder.hash_len, sizeof(HashString));
    decoder.data_hash = (DataHash*)calloc(decoder.hash_len, sizeof(DataHash));
    decoder.s_data_hash = (SortedDataHash*)calloc(decoder.hash_len, sizeof(SortedDataHash));
    decoder.count = 0;
    parse_hash_strings(decoder, s);
    quick_sort(decoder.s_data_hash, 0, decoder.hash_len - 1);
    dedup_sorted_data_hash(decoder);
    // for (size_t i = 0; i < decoder.hash_len; i++)
    // {
    //     printf("%zu,", i);
    //     print_sorted_data_hash(decoder.s_data_hash+i);
    // }
}
void free_decoder(Decoder& decoder)
{
    free(decoder.hash_string);
    decoder.hash_string = 0;
    free(decoder.data_hash);
    decoder.data_hash = 0;
    free(decoder.s_data_hash);
    decoder.s_data_hash = 0;
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
        printf("data md5 decoder [opencl], v1.0, by herman\n");
        printf("usage: mm filename\n");
        return 1;
    }

    Decoder decoder;
    init_decoder(decoder, s);
    free(s);
    s = 0;
    printf("find %zu hashes\n", decoder.hash_len);
    printf("they have %zu duplicated, %zu unique ones\n", decoder.hash_len - decoder.dedup_len, decoder.dedup_len);

    // allocate device memory
    size_t sdh_len = decoder.dedup_len * sizeof(SortedDataHash);
    cl_int error = CL_SUCCESS;
    cl_mem aBuffer = clCreateBuffer (context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sdh_len,
		decoder.s_data_hash, &error);
	CheckCLError (error);
    clSetKernelArg (kernel, 0, sizeof (cl_mem), &aBuffer);

    printf("0%% @%lds - 0/%zu\n", time(NULL) - start, decoder.dedup_len);
    // work on each prefix
    int type = DATA_TYPE_MOBILE;
    int len = decoder.dedup_len;
    for (size_t i = 0; i < PREFIX_SIZE; i++)
    {
        cl_uchar4 param1;
        param1.s[0] = PREFIX_LIST[i] / 100 + '0';
        param1.s[1] = (PREFIX_LIST[i] % 100) / 10 + '0';
        param1.s[2] = PREFIX_LIST[i] % 10 + '0';
        clSetKernelArg (kernel, 1, sizeof (int), &type);
        clSetKernelArg (kernel, 2, sizeof (int), &len);
        clSetKernelArg (kernel, 3, sizeof (cl_uchar4), &param1);
        // call opencl
        const size_t globalWorkSize [] = { SLICE_LEN, 0, 0 };
	    CheckCLError (clEnqueueNDRangeKernel (queue, kernel, 1,
            nullptr,
            globalWorkSize,
            nullptr,
            0, nullptr, nullptr));

        CheckCLError (clEnqueueReadBuffer (queue, aBuffer, CL_TRUE, 0,
            sdh_len,
            decoder.s_data_hash,
            0, nullptr, nullptr));

        decoder.count = 0;
        for (size_t h = 0; h < decoder.dedup_len; h++)
        {
            if (decoder.s_data_hash[h].data_hash.data[0])
            {
                decoder.count++;
            }
        }
        printf("\033[1A%zu/%zu @%lds - searching %lu%%...\n", decoder.count, decoder.dedup_len, time(NULL) - start, (i+1)*100/ PREFIX_SIZE);
        if (decoder.count == decoder.dedup_len)
        {
            break;
        }
    }

	clReleaseMemObject (aBuffer);

    // write reults
    printf("total %zu hashes are decoded\n", decoder.count);
    resort_data_hash(decoder);
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