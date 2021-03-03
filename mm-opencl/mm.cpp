#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <boost/program_options.hpp>

#include "compute.h"
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
        154, 146};
static const size_t PREFIX_SIZE = sizeof(PREFIX_LIST) / sizeof(PREFIX_LIST[0]);

bool read_from_file(const std::string &filename, std::string &str)
{
    std::ifstream file(filename);
    if (file.is_open())
    {
        std::ostringstream stream;
        stream << file.rdbuf();
        file.close();
        str = stream.str();
        return true;
    }
    else
    {
        return false;
    }
}

void write_to_file(const std::string &filename, std::string &str)
{
    std::string outfile(filename);
    outfile += ".out";
    std::ofstream file(outfile);
    file << str;
    file.close();
    std::cout << "please find results in file: " << outfile << std::endl;
}

void print_version()
{
    std::cout << "md5 decoder [opencl], v1.0, by herman\n";
}

void print_info(Compute& compute)
{
    std::cout << "printing opencl info...\n";
    int platform_index = 0;
    int device_index = 0;
    compute.benchmark(platform_index, device_index);
    compute.print_info();
    // write mm.ini
    std::ofstream file("mm.ini");
    file << "platform = " << platform_index << "\n";
    file << "device = " << device_index << "\n";
    file.close();
}

int main(int argc, char *argv[])
{
    // init compute engine
    Compute compute;

    // check mm.ini
    std::ifstream ini("mm.ini");
    if (!ini.good())
    {
        // run for the first time
        print_info(compute);
        return 0;
    }

    // handle program options
    int platform = 0;
    int device = 0;
    namespace po = boost::program_options;
    po::options_description desc("usage: mm [option] filename\noptions");
    desc.add_options()
        ("help,h", "print help message")
        ("version,v", "print version")
        ("info,i", "print opencl info")
        ("platform,p", po::value<int>(&platform), "select opencl platform index")
        ("device,d", po::value<int>(&device), "select opencl device index")
        ;
    po::positional_options_description p;
    p.add("input-file", -1);
    po::options_description cmdline_options;
    cmdline_options.add(desc).add_options()
        ("input-file", po::value< std::string >(), "input file")
        ;
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    po::store(po::parse_config_file("mm.ini", desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc ;
        return 0;
    }
    if (vm.count("version"))
    {
        print_version();
        return 0;
    }
    if (vm.count("info"))
    {
        print_info(compute);
        return 0;
    }
    if (!vm.count("input-file"))
    {
        std::cout << desc ;
        return 0;
    }

    // process hash file
    auto filename = vm["input-file"].as<std::string>();
    std::string str;
    if (!(read_from_file(filename, str)))
    {
        std::cout << "file " << filename << " not found\n";
        return 1;
    }

    print_version();

    Decoder decoder(str.c_str());
    const int hash_len = decoder.get_hash_len();
    const int dedup_len = decoder.get_dedup_len();
    std::cout << "find " << hash_len << " hashes (" << hash_len - dedup_len << " duplicated, " << dedup_len << " unique)\n";

    compute.set_device(platform, device);
    std::cout << "using " << compute.get_device_name(platform, device) << std::endl;
    Hash *p_hash = decoder.create_hash_buffer();
    compute.set_hash_buffer(p_hash, dedup_len);
    delete[] p_hash;

    // work on each prefix
    time_t start = time(NULL);
    std::cout << "starting...\n";
    int decode_count = 0;
    for (int i = 0; i < PREFIX_SIZE; i++)
    {
        cl_uchar4 prefix;
        prefix.s0 = PREFIX_LIST[i] / 100 + '0';
        prefix.s1 = (PREFIX_LIST[i] / 10) % 10 + '0';
        prefix.s2 = PREFIX_LIST[i] % 10 + '0';
        decode_count = compute.run(&prefix, dedup_len);
        std::cout << "\033[1A" << decode_count << "/" << dedup_len << " @" << time(NULL) - start << "s - searching " << (i + 1) * 100 / PREFIX_SIZE << "%...\n";
        if (decode_count == dedup_len)
        {
            break;
        }
    }
    std::cout << "total " << decode_count << " hashes are decoded\n";

    // read results
    MobileData *p_data = compute.read_results(dedup_len);
    decoder.update_result(p_data);
    delete[] p_data;
    compute.release_instance();

    // write reults
    std::string result;
    decoder.get_result(result);
    write_to_file(filename, result);
}