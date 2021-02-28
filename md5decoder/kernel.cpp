#include <iostream>
#include <chrono>
#include <cstring>

#include "kernel.h"
#include "compute_cl.h"

void _CheckCLError(cl_int error, int line)
{
    if (error != CL_SUCCESS)
    {
        std::cout << "OpenCL call failed with error " << error << " @" << line << std::endl;
        exit(1);
    }
}
#define CheckCLError(error) _CheckCLError(error, __LINE__);

Kernel::Kernel()
{
}

Kernel::~Kernel()
{
    clReleaseMemObject(hash_buffer);
    clReleaseMemObject(data_buffer);
    clReleaseMemObject(helper_buffer);
    clReleaseMemObject(params_buffer);
    clReleaseMemObject(input_buffer);

    clReleaseCommandQueue(queue);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);
}

void Kernel::enum_devices(Platforms &platforms)
{
    platforms.count = 0;
    clGetPlatformIDs(0, nullptr, &platforms.count);

    if (platforms.count == 0)
    {
        std::cout << "No OpenCL platform found" << std::endl;
        exit(1);
    }

    platforms.ids = new cl_platform_id[platforms.count];
    clGetPlatformIDs(platforms.count, platforms.ids, nullptr);

    platforms.names.resize(platforms.count);
    platforms.devices.resize(platforms.count);
    for (size_t i = 0; i < platforms.count; i++)
    {
        // get platform name
        size_t size = 0;
        clGetPlatformInfo(platforms.ids[i], CL_PLATFORM_NAME, 0, nullptr, &size);
        auto name = new char[size];
        clGetPlatformInfo(platforms.ids[i], CL_PLATFORM_NAME, size, name, nullptr);
        platforms.names[i] = name;
        delete[] name;

        // enum devices
        auto &devices = platforms.devices[i];
        devices.count = 0;
        clGetDeviceIDs(platforms.ids[i], CL_DEVICE_TYPE_ALL, 0, nullptr,
                       &devices.count);

        if (devices.count == 0)
        {
            std::cout << "No OpenCL devices found" << std::endl;
            continue;
        }

        devices.ids = new cl_device_id[devices.count];
        clGetDeviceIDs(platforms.ids[i], CL_DEVICE_TYPE_ALL, devices.count,
                       devices.ids, nullptr);

        devices.names.resize(devices.count);
        for (size_t j = 0; j < devices.count; j++)
        {
            size_t size = 0;
            clGetDeviceInfo(devices.ids[j], CL_DEVICE_NAME, 0, nullptr, &size);
            auto name = new char[size];
            clGetDeviceInfo(devices.ids[j], CL_DEVICE_NAME, size, name, nullptr);
            devices.names[j] = name;
            delete[] name;
        }
    }
}

void Kernel::release_platforms(Platforms &platforms)
{
    for (auto device : platforms.devices)
    {
        if (device.ids)
            delete[] device.ids;
    }
    if (platforms.ids)
        delete[] platforms.ids;
}

void Kernel::set_device(Platforms &platforms, int platform_index, int device_index)
{
    const cl_context_properties contextProperties[] =
        {
            CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platforms.ids[platform_index]),
            0, 0};

    cl_uint deviceIdCount = platforms.devices[platform_index].count;
    cl_device_id *deviceIds = platforms.devices[platform_index].ids;
    cl_int error = CL_SUCCESS;
    context = clCreateContext(contextProperties, deviceIdCount,
                              deviceIds, nullptr, nullptr, &error);
    CheckCLError(error);

    size_t lengths[1] = {strlen(compute_cl)};
    const char *sources[1] = {compute_cl};
    program = clCreateProgramWithSource(context, 1, sources, lengths, &error);
    CheckCLError(error);
    // program = CreateProgram("kernels/compute.cl", context);

    CheckCLError(clBuildProgram(program, deviceIdCount, deviceIds, nullptr, nullptr, nullptr));

    kernel = clCreateKernel(program, "compute", &error);
    CheckCLError(error);
#ifdef __APPLE__
    queue = clCreateCommandQueue(context, deviceIds[device_index],
                                 0, &error);
#else
    queue = clCreateCommandQueueWithProperties(context, deviceIds[device_index],
                                               0, &error);
#endif
    CheckCLError(error);
}

void Kernel::create_hash_buffer(void *p, int len)
{
    cl_int error = CL_SUCCESS;
    hash_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                 len,
                                 p, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &hash_buffer);
}
void Kernel::create_data_buffer(int len)
{
    cl_int error = CL_SUCCESS;
    data_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                 len,
                                 0, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &data_buffer);
}
void Kernel::create_helper_buffer(void *p, int len)
{
    cl_int error = CL_SUCCESS;
    helper_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   len,
                                   p, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &helper_buffer);
}
void Kernel::create_params_buffer(void *p, int len)
{
    cl_int error = CL_SUCCESS;
    params_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                len,
                                p, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &params_buffer);
}
void Kernel::create_input_buffer(int len)
{
    cl_int error = CL_SUCCESS;
    input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                 len,
                                 0, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &input_buffer);
}

int Kernel::run(void *input, int length, size_t kernel_work_size[3])
{
    CheckCLError(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
                                        length,
                                        input,
                                        0, nullptr, nullptr));

    CheckCLError(clEnqueueNDRangeKernel(queue, kernel, 3,
                                        nullptr,
                                        kernel_work_size,
                                        nullptr,
                                        0, nullptr, nullptr));

    int count = 0;
    CheckCLError(clEnqueueReadBuffer(queue, params_buffer, CL_TRUE, 0,
                                        sizeof(int),
                                        &count,
                                        0, nullptr, nullptr));
    return count;
}

void Kernel::read_results(void *p_data, int length)
{
    cl_int error = CL_SUCCESS;
    CheckCLError(clEnqueueReadBuffer(queue, data_buffer, CL_TRUE, 0,
                                     length,
                                     p_data,
                                     0, nullptr, nullptr));
}