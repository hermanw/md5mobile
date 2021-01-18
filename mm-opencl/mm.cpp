#include <stdio.h>
#include <time.h>
#include <string.h>

#ifdef __APPLE__
	#include "OpenCL/opencl.h"
#else
	#include "CL/cl.h"
#endif

#include "mm.h"
#include "decoder.h"

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
    std::string result;
    decoder->get_result(result);
    fputs(result.c_str(), f);
    fclose(f);
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

    // use default device
    device_id = deviceIds[1];
    size_t size = 0;
    clGetDeviceInfo (device_id, CL_DEVICE_NAME, 0, nullptr, &size);
    char *name = (char *)malloc(sizeof(char) * size);
    clGetDeviceInfo (device_id, CL_DEVICE_NAME, size, name, nullptr);
    printf("using %s\n", name);
    free(name);

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
    printf("md5 decoder [opencl], v1.0, by herman\n");
    // process hash file
    char* s = NULL;
    if (argc < 2 || !(s = read_from_file(argv[1])))
    {
        printf("usage: mm filename\n");
        return 1;
    }

    Decoder decoder(s);
    free(s);
    s = 0;
    const int hash_len = decoder.get_hash_len();
    const int dedup_len = decoder.get_dedup_len();
    printf("find %d hashes (%d duplicated, %d unique)\n", hash_len, hash_len - dedup_len, dedup_len);

    // setup OpenCL
    if(init_opencl())
    {
        exit(1);
    }

    // buffer0 for hash
    Hash* p_hash = decoder.create_hash_buffer();
    cl_int error = CL_SUCCESS;
    cl_mem buffer0 = clCreateBuffer (context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		dedup_len * sizeof(Hash),
		p_hash, &error);
	CheckCLError (error);
    clSetKernelArg (kernel, 0, sizeof (cl_mem), &buffer0);
    delete[] p_hash;

    // buffer1 for mobile data
    cl_mem buffer1 = clCreateBuffer (context, CL_MEM_WRITE_ONLY,
		dedup_len * sizeof(MobileData),
		0, &error);
	CheckCLError (error);
    clSetKernelArg (kernel, 1, sizeof (cl_mem), &buffer1);

    // buffer2 for number helper strings
    char* p_numbers = (char *)malloc(10000 * 4);
    for (size_t i = 0; i < 10000; i++)
    {
        p_numbers[i*4+0] = i/1000 + '0';
        p_numbers[i*4+1] = i/100%10 + '0';
        p_numbers[i*4+2] = i/10%10 + '0';
        p_numbers[i*4+3] = i%10 + '0';
    }
    cl_mem buffer2 = clCreateBuffer (context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		10000 * 4,
		p_numbers, &error);
	CheckCLError (error);
    clSetKernelArg (kernel, 2, sizeof (cl_mem), &buffer2);
    free(p_numbers);

    // buffer3 for params
    int params[5];
    params[0] = dedup_len;
    params[1] = 0;

    // work on each prefix
    time_t start = time(NULL);
    printf("starting...\n");
    for (int i = 0; i < PREFIX_SIZE; i++)
    {
        params[2] = PREFIX_LIST[i] / 100 + '0';
        params[3] = (PREFIX_LIST[i] / 10) % 10 + '0';
        params[4] = PREFIX_LIST[i] % 10 + '0';
        cl_mem buffer3 = clCreateBuffer (context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
            sizeof (int) * 5,
            params, &error);
        CheckCLError (error);
        clSetKernelArg (kernel, 3, sizeof (cl_mem), &buffer3);
        // call opencl
        const size_t globalWorkSize [] = { 10000, 10000, 0 };
	    CheckCLError (clEnqueueNDRangeKernel (queue, kernel, 2,
            nullptr,
            globalWorkSize,
            nullptr,
            0, nullptr, nullptr));

        CheckCLError (clEnqueueReadBuffer (queue, buffer3, CL_TRUE, 0,
            sizeof (int)*5,
		    params,
            0, nullptr, nullptr));
    	clReleaseMemObject (buffer3);

        printf("\033[1A%d/%d @%lds - searching %lu%%...\n", params[1], dedup_len, time(NULL) - start, (i+1)*100/ PREFIX_SIZE);
        if (params[1] == dedup_len)
        {
            break;
        }
    }
    printf("total %d hashes are decoded\n", params[1]);

    // read results
    MobileData* p_m_data = (MobileData*)calloc(dedup_len, sizeof(MobileData));
    CheckCLError (clEnqueueReadBuffer (queue, buffer1, CL_TRUE, 0,
        dedup_len * sizeof(MobileData),
        p_m_data,
        0, nullptr, nullptr));
    decoder.update_result(p_m_data);
    free(p_m_data);
    p_m_data = 0;

    // write reults
    size_t fn_len = strlen(argv[1]) + 5;
    char* outfile = (char*)malloc(fn_len);
    strcpy(outfile, argv[1]);
    strcat(outfile, ".out");
    write_to_file(outfile, &decoder);
    printf("please find results in file: %s\n", outfile);
    free(outfile);

    // release resources
	clReleaseMemObject (buffer2);
    clReleaseMemObject (buffer1);
    clReleaseMemObject (buffer0);

    release_opencl();
}