#include <vector>
#include <string>

#define CL_TARGET_OPENCL_VERSION 200

#ifdef __APPLE__
#include "OpenCL/opencl.h"
#else
#include "CL/cl.h"
#endif

#include "decoder.h"

typedef struct
{
    cl_uint count;
    cl_device_id *ids;
    std::vector<std::string> names;
    std::vector<long> scores;
} Devices;

typedef struct
{
    cl_uint count;
    cl_platform_id *ids;
    std::vector<std::string> names;
    std::vector<Devices> devices;
} Platforms;

class Compute
{
private:
    Platforms platforms;

    // instance
    cl_context context;
    cl_program program;
    cl_kernel kernel;
    cl_command_queue queue;
    cl_mem buffer0;
    cl_mem buffer1;
    cl_mem buffer2;
    cl_mem buffer3;

public:
    Compute();
    ~Compute();
    void enum_devices();
    void print_info();
    void set_device(int platform_index, int device_index);
    const std::string &get_device_name(int platform_index, int device_index) const;
    void set_hash_buffer(Hash *p_hash, int dedup_len);
    int run(cl_uchar4* prefix, int dedup_len);
    MobileData *read_results(int dedup_len);
    void release_instance();
    void benchmark(int &platform_index, int &device_index);
};