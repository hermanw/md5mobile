#ifndef __DEVICES_H
#define __DEVICES_H

class Devices
{
public:
    cl_uint count;
    cl_device_id* ids;
    std::vector<std::string> names;
    std::vector<long> scores;

public:
    ~Devices()
    {
        if (ids)
            delete[] ids;
    }
};

#endif