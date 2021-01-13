#include <stdio.h>
#include <time.h>

#include <vuh/array.hpp>
#include <vuh/vuh.h>
#include "decoder.h"

// this project is derived from mm-opencl

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
154, 146
};
static const size_t PREFIX_SIZE = sizeof(PREFIX_LIST) / sizeof(PREFIX_LIST[0]);

char* read_from_file(char* filename)
{
    FILE* f = fopen(filename, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buffer = (char*)malloc(len + 1);
        len = fread(buffer, 1, len, f);
        fclose(f);
        buffer[len] = 0;
        return buffer;
    }
    return 0;
}

void write_to_file(char* filename, Decoder* decoder)
{
    FILE* f = fopen(filename, "w");
    for (int h = 0; h < decoder->hash_len; h++)
    {
        for (size_t i = 0; i < HASH_LEN; i++)
        {
            fputc(decoder->hash_string[h][i], f);
        }
        fputc(',', f);
        for (size_t i = 0; i < MOBILE_LEN; i++)
        {
            fputc(decoder->m_data[h].value[i], f);
        }
        fputc('\n', f);
    }
    fclose(f);
}

int main(int argc, char* argv[])
{
    printf("md5 decoder [vulkan], v1.0, by herman\n");
    // process hash file
    char* s = NULL;
    if (argc < 2 || !(s = read_from_file(argv[1])))
    {
        printf("usage: mm filename\n");
        return 1;
    }

    Decoder decoder;
    init_decoder(&decoder, s);
    free(s);
    s = 0;
    printf("find %d hashes (%d duplicated, %d unique)\n", decoder.hash_len, decoder.hash_len - decoder.dedup_len, decoder.dedup_len);

    // setup vulkan device
    auto instance = vuh::Instance();
	auto device = instance.devices()[0];
    auto program = vuh::Program<vuh::typelist<>, int>(device, "compute.spv");
    program.grid(10000,10000);
    printf("using %s\n", device.properties().deviceName.data());

    // allocate device memory
    auto v_hash = std::vector<Hash>();
    for (int i = 0; i < decoder.dedup_len; i++)
    {
        v_hash.push_back(decoder.s_hash[i].hash);
    }
    auto d_buffer0 = vuh::Array<Hash>(device, v_hash);
    auto d_buffer1 = vuh::Array<MobileData>(device, decoder.dedup_len);
    auto numbers = std::vector<char>();
    for (size_t i = 0; i < 10000; i++)
    {
        numbers.push_back(i/1000 + '0');
        numbers.push_back(i/100%10 + '0');
        numbers.push_back(i/10%10 + '0');
        numbers.push_back(i%10 + '0');
    }
    auto d_buffer2 = vuh::Array<char>(device, numbers);
    auto params = std::vector<int>(5);
    params[0] = decoder.dedup_len;
    params[1] = 0;
    auto d_buffer3 = vuh::Array<int>(device, 5);

    // work on each prefix
    time_t start = time(NULL);
    printf("starting...\n");
    for (int i = 0; i < PREFIX_SIZE; i++)
    {
		params[2] = PREFIX_LIST[i] / 100 + '0';
        params[3] = (PREFIX_LIST[i] / 10) % 10 + '0';
        params[4] = PREFIX_LIST[i] % 10 + '0';
        d_buffer3.fromHost(begin(params), end(params));

        program.run(0, d_buffer0, d_buffer1, d_buffer2, d_buffer3);
        d_buffer3.toHost(begin(params));
        
        printf("\033[1A%d/%d @%lds - searching %lu%%...\n", params[1], decoder.dedup_len, time(NULL) - start, (i+1)*100/ PREFIX_SIZE);
        if (params[1] == decoder.dedup_len)
        {
            break;
        }
    }
    printf("total %d hashes are decoded\n", params[1]);

    // read results
    auto v_data = std::vector<MobileData>(decoder.dedup_len);
    d_buffer1.toHost(begin(v_data));
    MobileData* p_m_data = (MobileData*)calloc(decoder.dedup_len, sizeof(MobileData));
    for (int i = 0; i < decoder.dedup_len; i++)
    {
        p_m_data[i] = v_data[i];
    }
    resort_data(&decoder, p_m_data);
    free(p_m_data);
    p_m_data = 0;

    // write reults
    size_t fn_len = strlen(argv[1]) + 5;
    char* outfile = (char*)malloc(fn_len);
    strcpy(outfile, argv[1]);
    strcat(outfile, ".out");
    write_to_file(outfile, &decoder);
    printf("please find results in file: %s\n", outfile);
    free(outfile);

    // release resources
    free_decoder(&decoder);
}