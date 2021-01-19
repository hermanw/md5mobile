#ifndef __PLATFORMS_H
#define __PLATFORMS_H

class Platforms
{
public:
    cl_uint count;
    cl_platform_id *ids;
    std::vector<std::string> names;
    std::vector<Devices> devices;

public:
    ~Platforms()
    {
        if (ids)
            delete[] ids;
    }
};

#endif