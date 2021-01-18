#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define HASH_LEN 32
#define MOBILE_LEN 11
#define BLOCK_LEN 64 // In bytes
#define STATE_LEN 4  // In words

typedef char HashString[HASH_LEN];

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
public:
    int hash_len;
    int dedup_len;
    HashString *hash_string;
    SortedHash *s_hash;
    MobileData *m_data;

public:
    Decoder(const char *s);
    ~Decoder();
    void resort_data(MobileData *p_m_data);

private:
    int is_valid_digit(const char c);
    char hexToNibble(char n);
    void hex_to_bytes(uint8_t *to, const char *from, int len);
    void update_hash(const char *hash_string, int index);
    int parse_hash_strings(bool is_update, const char *s);
    int compare_hash(const uint32_t *a, const uint32_t *b);
    void quick_sort(SortedHash *array, int from, int to);
    void dedup_sorted_hash();
};
