#include "decoder.h"

int Decoder::is_valid_digit(const char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
char Decoder::hexToNibble(char n)
{
    if (n >= 'a' && n <= 'f')
    {
        return n - 'a' + 10;
    }
    else if (n >= 'A' && n <= 'F')
    {
        return n - 'A' + 10;
    }
    else
    {
        return n - '0';
    }
}
void Decoder::hex_to_bytes(uint8_t *to, const char *from, int len)
{
    for (int i = 0; i < len / 2; i++)
    {
        to[i] = (hexToNibble(from[i * 2]) << 4) + hexToNibble(from[i * 2 + 1]);
    }
}

void Decoder::update_hash(const char *a_hash_string, int index)
{
    memcpy(hash_string[index], a_hash_string, HASH_LEN);
    hex_to_bytes((uint8_t *)(s_hash[index].hash.value), a_hash_string, HASH_LEN);
    s_hash[index].index = index;
}

int Decoder::parse_hash_strings(bool is_update, const char *s)
{
    int valid_digit_num = 0;
    int count = 0;
    char a_hash_string[HASH_LEN];
    while (*s)
    {
        if (*s == ',')
        {
            if (valid_digit_num == HASH_LEN)
            {
                if (is_update)
                {
                    update_hash(a_hash_string, count);
                }
                count++;
            }
            valid_digit_num = 0;
        }
        else if (is_valid_digit(*s))
        {
            if (is_update)
            {
                a_hash_string[valid_digit_num] = *s;
            }
            valid_digit_num++;
        }
        s++;
    }
    if (valid_digit_num == HASH_LEN)
    {
        if (is_update)
        {
            update_hash(a_hash_string, count);
        }
        count++;
    }
    return count;
}

int Decoder::compare_hash(const uint32_t *a, const uint32_t *b)
{
    for (int i = 0; i < STATE_LEN; i++)
    {
        if (a[i] < b[i])
        {
            return -1;
        }
        else if (a[i] > b[i])
        {
            return 1;
        }
    }

    return 0;
}

void Decoder::quick_sort(SortedHash *array, int from, int to)
{
    if (from >= to)
        return;
    SortedHash temp;
    int i = from, j;
    for (j = from + 1; j <= to; j++)
    {
        if (compare_hash(array[j].hash.value, array[from].hash.value) < 0)
        {
            i = i + 1;
            temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
    }

    temp = array[i];
    array[i] = array[from];
    array[from] = temp;
    quick_sort(array, from, i - 1);
    quick_sort(array, i + 1, to);
}

void Decoder::dedup_sorted_hash()
{
    dedup_len = hash_len;
    for (int i = 1; i < dedup_len; i++)
    {
        if (compare_hash(s_hash[i].hash.value, s_hash[i - 1].hash.value) == 0)
        {
            SortedHash temp = s_hash[i];
            temp.index_dup = s_hash[i - 1].index;
            for (int j = i; j < hash_len - 1; j++)
            {
                s_hash[j] = s_hash[j + 1];
            }
            s_hash[hash_len - 1] = temp;
            dedup_len--;
            i--;
        }
    }
}

void Decoder::resort_data(MobileData *p_m_data)
{
    for (int i = 0; i < hash_len; i++)
    {
        int index = s_hash[i].index;
        if (i < dedup_len)
        {
            m_data[index] = p_m_data[i];
        }
        else
        {
            int index_dup = s_hash[i].index_dup;
            m_data[index] = m_data[index_dup];
        }
    }
}

Decoder::Decoder(const char *s)
{
    hash_len = parse_hash_strings(false, s);
    hash_string = (HashString *)calloc(hash_len, sizeof(HashString));
    s_hash = (SortedHash *)calloc(hash_len, sizeof(SortedHash));
    parse_hash_strings(true, s);
    quick_sort(s_hash, 0, hash_len - 1);
    dedup_sorted_hash();
    m_data = (MobileData *)calloc(hash_len, sizeof(MobileData));
}

Decoder::~Decoder()
{
    free(hash_string);
    hash_string = 0;
    free(s_hash);
    s_hash = 0;
    free(m_data);
    m_data = 0;
}
