#include <vector>
#include <map>
#include "nlohmann/json.hpp"

enum ds_type {ds_type_list, ds_type_digit, ds_type_idsum, ds_type_char};

struct DataSection
{
    ds_type type;
    int index;
    int length;
    std::string source;
};

class Cfg
{
public:
    int length;
    std::vector<struct DataSection> cpu_sections;
    std::vector<struct DataSection> gpu_sections;
    std::map<std::string, std::vector<std::string>> sources;
    size_t kernel_work_size[3] = {1, 1, 1};

    Cfg(const char *filename, const char *cfg_name);

private:
    void set_data_section(DataSection &ds, nlohmann::json &item);
};