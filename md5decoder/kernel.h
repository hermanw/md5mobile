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
    cl_mem helper_buffer;
    cl_mem params_buffer;

public:
    Kernel();
    ~Kernel();
    static void enum_devices(Platforms &platforms);
    static void release_platforms(Platforms &platforms);
    
    void set_device(Platforms &platforms, int platform_index, int device_index);
    void create_hash_buffer(void *p, int len);
    void create_data_buffer(int len);
    void create_helper_buffer(void *p, int len);
    void create_params_buffer(int len);
    void run(int* params, int length, size_t kernel_work_size[3]);
    void read_results(void* p_data, int length);
};