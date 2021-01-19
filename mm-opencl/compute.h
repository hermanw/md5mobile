#include <vector>
#include <string>

#ifdef __APPLE__
#include "OpenCL/opencl.h"
#else
#include "CL/cl.h"
#endif

#include "devices.h"
#include "platforms.h"
#include "decoder.h"

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

public:
    Compute();
    ~Compute();
    void enum_devices();
    void print_info();
    void set_device(int platform_index, int device_index);
    const std::string& get_device_name(int platform_index, int device_index) const;
    void set_hash_buffer(Hash *p_hash, int dedup_len);
    void run(int params[5]);
    MobileData* read_results(int dedup_len);
    void release_instance();
    void benchmark(int &platform_index, int &device_index);
};