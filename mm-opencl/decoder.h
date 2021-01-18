#include <vector>
#include <string>

#define HASH_LEN 32
#define MOBILE_LEN 11
#define BLOCK_LEN 64 // In bytes
#define STATE_LEN 4  // In words

typedef struct
{
    uint32_t value[STATE_LEN];
} Hash;

typedef struct
{
    uint8_t value[MOBILE_LEN];
} MobileData;

typedef struct
{
    int index;
    int index_dup;
    Hash hash;
} SortedHash;

class Decoder
{
private:
    int hash_len;
    int dedup_len;
    std::vector<std::string> hash_string;
    std::vector<SortedHash> s_hash;
    std::vector<std::string> m_data;

public:
    Decoder(const char *s);
    int get_hash_len() const { return hash_len; }
    int get_dedup_len() const { return dedup_len; }
    Hash *create_hash_buffer();
    void update_result(MobileData *p_m_data);
    void get_result(std::string& result);

private:
    int is_valid_digit(const char c);
    char hexToNibble(char n);
    void hex_to_bytes(uint8_t *to, const char *from, int len);
    void update_hash(const char *hash_string, int index);
    void parse_hash_strings(const char *s);
    static int compare_hash_binary(const uint32_t *a, const uint32_t *b);
    static bool compare_hash(SortedHash &a, SortedHash &b);
    void dedup_sorted_hash();
};
