#include "decoder.h"

int is_valid_digit(const char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
char hexToNibble(char n)
{
    if(n >= 'a' && n <= 'f')
    {
        return n - 'a' + 10;
    }
    else if(n >= 'A' && n <= 'F')
    {
        return n - 'A' + 10;
    }
    else
    {
        return n - '0';
    }
}
void hex_to_bytes(uint8_t* to, const char* from, int len)
{
    for (int i = 0; i < len / 2; i++)
    {
        to[i] = (hexToNibble(from[i * 2]) << 4) + hexToNibble(from[i * 2 + 1]);
    }
}

void update_hash(Decoder* decoder, const char* hash_string, int index)
{
    memcpy(decoder->hash_string[index], hash_string, HASH_LEN);
    hex_to_bytes((uint8_t*)(decoder->s_hash[index].hash.value), hash_string, HASH_LEN);
    decoder->s_hash[index].index = index;
}

int parse_hash_strings(Decoder* decoder, const char* s)
{
    int valid_digit_num = 0;
    int count = 0;
    char hash_string[HASH_LEN];
    while (*s)
    {
        if (*s == ',')
        {
            if (valid_digit_num == HASH_LEN)
            {
                if (decoder)
                {
                    update_hash(decoder, hash_string, count);
                }
                count++;
            }
            valid_digit_num = 0;
        }
        else if (is_valid_digit(*s))
        {
            if(decoder)
            {
                hash_string[valid_digit_num] = *s;
            }
            valid_digit_num++;
        }
        s++;
    }
    if (valid_digit_num == HASH_LEN)
    {
        if (decoder)
        {
            update_hash(decoder, hash_string, count);
        }
        count++;
    }
    return count;
}

int compare_hash(const uint32_t* a, const uint32_t* b)
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

void quick_sort(SortedHash* array, int from, int to)
{
    if (from >= to)return;
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

void dedup_sorted_hash(Decoder* decoder)
{
    decoder->dedup_len = decoder->hash_len;
    for (int i = 1; i < decoder->dedup_len; i++)
    {
        if (compare_hash(decoder->s_hash[i].hash.value, decoder->s_hash[i - 1].hash.value) == 0)
        {
            SortedHash temp = decoder->s_hash[i];
            temp.index_dup = decoder->s_hash[i - 1].index;
            for (int j = i; j < decoder->hash_len - 1; j++)
            {
                decoder->s_hash[j] = decoder->s_hash[j + 1];
            }
            decoder->s_hash[decoder->hash_len - 1] = temp;
            decoder->dedup_len--;
            i--;
        }
    }
}

void resort_data(Decoder* decoder, MobileData* p_m_data)
{
    for (int i = 0; i < decoder->hash_len; i++)
    {
        int index = decoder->s_hash[i].index;
        if (i < decoder->dedup_len)
        {
            decoder->m_data[index] = p_m_data[i];
        }
        else
        {
            int index_dup = decoder->s_hash[i].index_dup;
            decoder->m_data[index] = decoder->m_data[index_dup];
        }
    }
}

void init_decoder(Decoder* decoder, const char* s)
{
    decoder->hash_len = parse_hash_strings(0, s);
    decoder->hash_string = (HashString*)calloc(decoder->hash_len, sizeof(HashString));
    decoder->s_hash = (SortedHash*)calloc(decoder->hash_len, sizeof(SortedHash));
    parse_hash_strings(decoder, s);
    quick_sort(decoder->s_hash, 0, decoder->hash_len - 1);
    dedup_sorted_hash(decoder);
    decoder->m_data = (MobileData*)calloc(decoder->hash_len, sizeof(MobileData));
    decoder->count = 0;
}

void free_decoder(Decoder* decoder)
{
    free(decoder->hash_string);
    decoder->hash_string = 0;
    free(decoder->s_hash);
    decoder->s_hash = 0;
    free(decoder->m_data);
    decoder->m_data = 0;
}
