#ifndef __DECODER_H
#define __DECODER_H

#include <vector>
#include <string>
#include "kernel.h"
#include "cfg.h"

#define HASH_LEN 32
#define BLOCK_LEN 64 // In bytes
#define STATE_LEN 2  // In dwords

typedef struct
{
    uint64_t value[STATE_LEN];
} Hash;

typedef struct
{
    int index;
    int index_dup;
    Hash hash;
} SortedHash;

class Decoder
{
private:
    Platforms m_platforms;
    Cfg *m_cfg;
    int m_hash_len;
    int m_dedup_len;
    std::vector<std::string> m_hash_string;
    std::vector<SortedHash> m_hash;
    std::vector<std::string> m_data;
    time_t m_start;
    int m_iterations;
    int m_iterations_len;
    bool m_benchmark;
    long m_kernel_score;

public:
    Decoder(const char *cfg_filename, const char *cfg_name);
    ~Decoder();
    void set_hash_string(const char *s);
    int get_hash_len() const { return m_hash_len; }
    int get_dedup_len() const { return m_dedup_len; }
    Hash *create_hash_buffer();
    void update_result(char *p_data, int data_length);
    void get_result(std::string &result);
    void decode(int platform_index, int device_index);
    void benchmark(int &platform_index, int &device_index);

private:
    int is_valid_digit(const char c);
    char hexToNibble(char n);
    void hex_to_bytes(uint8_t *to, const char *from, int len);
    void update_hash(const char *hash_string, int index);
    void parse_hash_string(const char *s);
    static int compare_hash_binary(const uint64_t *a, const uint64_t *b);
    static bool compare_hash(SortedHash &a, SortedHash &b);
    void dedup_sorted_hash();
    bool run_in_host(Kernel *kernel, uint8_t *params, int index);
    bool run_in_kernel(Kernel *kernel, uint8_t *params);
};

#endif