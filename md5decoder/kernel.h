#include <vector>
#include <string>

#define CL_TARGET_OPENCL_VERSION 200

#ifdef __APPLE__
#include "OpenCL/opencl.h"
#else
#include "CL/cl.h"
#endif

typedef struct
{
    cl_uint count;
    cl_device_id *ids;
    std::vector<std::string> names;
} Devices;

typedef struct
{
    cl_uint count;
    cl_platform_id *ids;
    std::vector<std::string> names;
    std::vector<Devices> devices;
} Platforms;

class Kernel
{
private:
    cl_context context;
    cl_program program;
    cl_kernel kernel;
    cl_command_queue queue;
    cl_mem hash_buffer;
    cl_mem data_buffer;
    cl_mem number_buffer;
    cl_mem helper_buffer;
    cl_mem count_buffer;
    cl_mem input_buffer;

public:
    Kernel();
    ~Kernel();
    static void enum_devices(Platforms &platforms);
    static void release_platforms(Platforms &platforms);
    
    void set_device(Platforms &platforms, int platform_index, int device_index, const char *options);
    void create_buffers(void* p_hash, void* p_number, void* p_helper, int hash_length, int data_length,int helper_length);
    int run(void *input, int hash_length, int data_length, size_t kernel_work_size[3]);
    void read_results(void* p_data, int length);
};