#include <iostream>
#include <chrono>
#include <cstring>

#include "compute.h"
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

Compute::Compute()
{
    enum_devices();
}

Compute::~Compute()
{
    for (auto device: platforms.devices)
    {
        if (device.ids)
            delete[] device.ids;
    }
    if (platforms.ids)
            delete[] platforms.ids;
}

void Compute::enum_devices()
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
        auto& devices = platforms.devices[i];
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
        devices.scores.resize(devices.count);
        for (size_t j = 0; j < devices.count; j++)
        {
            size_t size = 0;
            clGetDeviceInfo (devices.ids[j], CL_DEVICE_NAME, 0, nullptr, &size);
            auto name = new char[size];
            clGetDeviceInfo (devices.ids[j], CL_DEVICE_NAME, size, name, nullptr);
            devices.names[j] = name;
            delete[] name;
        }
    }
}

void Compute::print_info()
{
    for (size_t i = 0; i < platforms.count; i++)
    {
        std::cout << "platform " << i << ": " << platforms.names[i] << std::endl;
        auto& devices = platforms.devices[i];
        for (size_t j = 0; j < devices.count; j++)
        {
            std::cout << "\tdevice " << j << ": " << devices.names[j] << ", score(" << devices.scores[j] << ")\n";
        }
    }
}

const std::string& Compute::get_device_name(int platform_index, int device_index) const
{
    return platforms.devices[platform_index].names[device_index];
}

void Compute::set_device(int platform_index, int device_index)
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

void Compute::release_instance()
{
    clReleaseMemObject(buffer2);
    clReleaseMemObject(buffer1);
    clReleaseMemObject(buffer0);

    clReleaseCommandQueue(queue);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseContext(context);
}

void Compute::set_hash_buffer(Hash *p_hash, int dedup_len)
{
    cl_int error = CL_SUCCESS;
    // buffer0 for hash
    buffer0 = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                             dedup_len * sizeof(Hash),
                             p_hash, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer0);

    // buffer1 for mobile data
    buffer1 = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                             dedup_len * sizeof(MobileData),
                             0, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer1);

    // buffer2 for number helper strings
    char *p_numbers = new char[10000 * 4];
    for (size_t i = 0; i < 10000; i++)
    {
        p_numbers[i * 4 + 0] = i / 1000 + '0';
        p_numbers[i * 4 + 1] = i / 100 % 10 + '0';
        p_numbers[i * 4 + 2] = i / 10 % 10 + '0';
        p_numbers[i * 4 + 3] = i % 10 + '0';
    }
    buffer2 = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                             10000 * 4,
                             p_numbers, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer2);
    delete[] p_numbers;
}

void Compute::run(int params[5])
{
    cl_int error = CL_SUCCESS;
    cl_mem buffer3 = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                    sizeof(int) * 5,
                                    params, &error);
    CheckCLError(error);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &buffer3);
    // call opencl
    const size_t globalWorkSize[] = {10000, 10000, 0};
    CheckCLError(clEnqueueNDRangeKernel(queue, kernel, 2,
                                        nullptr,
                                        globalWorkSize,
                                        nullptr,
                                        0, nullptr, nullptr));

    CheckCLError(clEnqueueReadBuffer(queue, buffer3, CL_TRUE, 0,
                                     sizeof(int) * 5,
                                     params,
                                     0, nullptr, nullptr));
    clReleaseMemObject(buffer3);
}

MobileData *Compute::read_results(int dedup_len)
{
    MobileData *p_data = new MobileData[dedup_len];
    CheckCLError(clEnqueueReadBuffer(queue, buffer1, CL_TRUE, 0,
                                     dedup_len * sizeof(MobileData),
                                     p_data,
                                     0, nullptr, nullptr));

    return p_data;
}

void Compute::benchmark(int &platform_index, int &device_index)
{
    long top_score = 0;
    for (size_t i = 0; i < platforms.count; i++)
    {
        auto& devices = platforms.devices[i];
        for (size_t j = 0; j < devices.count; j++)
        {
            Decoder decoder("c1f80bc82197a495ee91fb37e2a0fda2");    // 18612341234
            const int hash_len = decoder.get_hash_len();
            const int dedup_len = decoder.get_dedup_len();

            auto start = std::chrono::steady_clock::now();

            set_device(i, j);
            Hash *p_hash = decoder.create_hash_buffer();
            set_hash_buffer(p_hash, dedup_len);
            delete[] p_hash;

            // params
            int params[5];
            params[0] = dedup_len;
            params[1] = 0;
            params[2] = '1';
            params[3] = '8';
            params[4] = '8';

            run(params);

            // read results
            MobileData *p_data = read_results(dedup_len);
            decoder.update_result(p_data);
            delete[] p_data;
            release_instance();

            auto end = std::chrono::steady_clock::now();
            long e = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            long score = 100000000/e;
            devices.scores[j] = score;
            if(score > top_score)
            {
                platform_index = i;
                device_index = j;
            }

            // // verify
            // std::string result;
            // decoder.get_result(result);
            // std::cout << result << std::endl;
        }
    }
}