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
    hash_string.push_back(std::string(a_hash_string));
    SortedHash sh;
    sh.index = index;
    hex_to_bytes((uint8_t *)(sh.hash.value), a_hash_string, HASH_LEN);
    s_hash.push_back(sh);
}

void Decoder::parse_hash_strings(const char *s)
{
    int valid_digit_num = 0;
    int count = 0;
    char a_hash_string[HASH_LEN + 1] = {0};
    while (*s)
    {
        if (*s == ',')
        {
            if (valid_digit_num == HASH_LEN)
            {
                update_hash(a_hash_string, count);
                count++;
            }
            valid_digit_num = 0;
        }
        else if (is_valid_digit(*s))
        {
            a_hash_string[valid_digit_num] = *s;
            valid_digit_num++;
        }
        s++;
    }
    if (valid_digit_num == HASH_LEN)
    {
        update_hash(a_hash_string, count);
        count++;
    }
    hash_len = count;
}

int Decoder::compare_hash_binary(const uint32_t *a, const uint32_t *b)
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

bool Decoder::compare_hash(SortedHash &a, SortedHash &b)
{
    return compare_hash_binary(a.hash.value, b.hash.value) < 0;
}

void Decoder::dedup_sorted_hash()
{
    dedup_len = hash_len;
    for (int i = 1; i < dedup_len; i++)
    {
        if (compare_hash_binary(s_hash[i].hash.value, s_hash[i - 1].hash.value) == 0)
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

Hash *Decoder::create_hash_buffer()
{
    Hash *p_hash = new Hash[dedup_len];
    for (int i = 0; i < dedup_len; i++)
    {
        p_hash[i] = s_hash[i].hash;
    }
    return p_hash;
}

void Decoder::update_result(MobileData *p_m_data)
{
    m_data.resize(hash_len);
    for (int i = 0; i < hash_len; i++)
    {
        int index = s_hash[i].index;
        if (i < dedup_len)
        {
            for (int j = 0; j < MOBILE_LEN; j++)
            {
                m_data[index] += p_m_data[i].value[j];
            }
        }
        else
        {
            int index_dup = s_hash[i].index_dup;
            m_data[index] = m_data[index_dup];
        }
    }
}

void Decoder::get_result(std::string& result)
{
    for (int h = 0; h < hash_len; h++)
    {
        result += hash_string[h];
        result += ',';
        result += m_data[h];
        result += '\n';
    }
}

Decoder::Decoder(const char *s)
{
    parse_hash_strings(s);
    std::sort(s_hash.begin(), s_hash.end(), compare_hash);
    dedup_sorted_hash();
}
