/*
BSD 3-Clause License

Copyright (c) 2020, _Xiangqian
        All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
        FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
        CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "bloomfilter.h"

uint64_t bitCount(uint64_t i)
{
    uint64_t c = 0;

    for (; i != 0; i = i >> 1) {
        if ((i&1) == 1) {
            c++;
        }
    }

    return c;
}

int BitsGet(uint64_t *data, uint64_t bit_index)
{
    //printf("bitsGET %lu, %lu, %lu, %lu, %lu, %lu, %lu\n", bit_index, bit_index >> 6, data[bit_index >> 6], (uint64_t) 1 << (bit_index & 63), bit_index & 63, 1 << (bit_index & 63), (data[bit_index >> 6] & ((uint64_t) 1 << (bit_index & 63))));
    return (data[bit_index >> 6] & ((uint64_t) 1 << (bit_index & 63))) != 0;
}

int BitsSet(BloomFilter *bf, uint64_t bit_index)
{
    uint64_t long_index = 0;
    uint64_t mask       = 0;
    uint64_t *data      = (uint64_t *)((void *)bf->bitset + HEADER_LEN);

    if (BitsGet(data, bit_index)) {
        return 0;
    }

    long_index = bit_index >> 6;
    mask = (uint64_t)1 << (bit_index & 63);

    for (int i = 0; i < bf->bitset->hash_num; i++) {
        uint64_t old = *(data + long_index);
        uint64_t new = old | mask;

        if (old == new) {
            return 0;
        }

        //TODO:automic
        if (__sync_bool_compare_and_swap(data + long_index, old, new) == 1) {
            break;
        }
    }

    //TOOD:atomic
    bf->bit_count ++;

    return 1;
}

uint64_t OptimalNumOfBits(uint64_t n, double p)
{
    if (p == 0) {
        p = 4.940656458412465441765687928682213723651e-324;
    }

    return (uint64_t)(-1 * (double)n * log(p) / (log(2) * log(2)));
}

//TODO:check
int OptimalNumOfHash(uint64_t n, uint64_t m)
{
    return (int)(fmax(1, floor(((double)(m / n) * log(2)) + 0.5)));
}

uint8_t * Serialized(BloomFilter *bf)
{
    BitSetHeader *bitset        = NULL;
    uint64_t *data              = NULL;
    uint32_t length             = 0;

    if (bf == NULL) {
        return NULL;
    }

    data = (uint64_t *)((void *)bf->bitset + HEADER_LEN);
    length = bf->bitset->length;

    for (int i = 0; i < length; i++) {
        *(data + i) = BF_HTONLL( *(data + i));
    }

    return (uint8_t *)bf->bitset;
}

BloomFilter *NewBF(uint64_t expect, double fpp)
{
    BloomFilter *bloomFilter    = NULL;
    BitSetHeader *bitset        = NULL;
    uint64_t bit_size           = 0;
    int length                  = 0;


    bit_size = OptimalNumOfBits(expect, fpp);

    length = (int)(ceil((double)bit_size / 64.0));
    if (length <= 0) {
        return NULL;
    }

    bitset = (BitSetHeader *)malloc(sizeof(uint64_t) * length + HEADER_LEN);
    memset(bitset, 0, sizeof(uint64_t) * length + HEADER_LEN);

    bitset->magic = 1;
    bitset->hash_num = OptimalNumOfHash(expect, bit_size);
    bitset->length = length;

    bloomFilter = (BloomFilter *)malloc(sizeof(BloomFilter));
    bloomFilter->seed = 0;
    bloomFilter->bit_count = 0;
    bloomFilter->hash_func = MurmurHash3_x64_128;
    bloomFilter->bitset = bitset;

    //printf("num_hash=%d, length=%d, bitcount=%llu, bit_size=%lu\n", bloomFilter->bitset->hash_num, bloomFilter->bitset->length, bloomFilter->bit_count, bit_size);

    return bloomFilter;
}

BloomFilter *LoadBF(void *byte_array, double array_len)
{
    BitSetHeader *src_bitset= (BitSetHeader *)(int8_t *)byte_array;
    uint32_t length         = BF_HTONL(src_bitset->length);
    uint64_t bitcount       = 0;
    BloomFilter *bloomFilter= NULL;
    BitSetHeader *bitset    = NULL;
    uint64_t *src_data      = NULL;
    uint64_t *dst_data      = NULL;

    bitset = (BitSetHeader *)malloc(sizeof(uint64_t) * length + HEADER_LEN);
    memset(bitset, 0, sizeof(uint64_t) * length + HEADER_LEN);

    bitset->magic = src_bitset->magic;
    bitset->hash_num = src_bitset->hash_num;
    bitset->length = length;

    dst_data = (uint64_t *)((void *)bitset + HEADER_LEN);
    src_data = (uint64_t *)((uint8_t *)byte_array + HEADER_LEN);

    for (int i = 0; i < length; i++) {
        *(dst_data + i) = BF_NTOHLL( *(src_data + i));
        bitcount += bitCount(*(dst_data + i));
    }

    bloomFilter = (BloomFilter *)malloc(sizeof(BloomFilter));
    bloomFilter->seed = 0;
    bloomFilter->bit_count = bitcount;
    bloomFilter->hash_func = MurmurHash3_x64_128;
    bloomFilter->bitset = bitset;

    //printf("num_hash=%d, length=%d, bitcount=%llu\n", bloomFilter->bitset->hash_num, bloomFilter->bitset->length, bloomFilter->bit_count);

    return bloomFilter;
}

void DestroyBF(BloomFilter *bf)
{
    if(bf != NULL) {
        if (bf->bitset != NULL) {
            free(bf->bitset);
        }
        free(bf);
    }
}

int PutUint64(BloomFilter *bf, double sn)
{
    uint8_t byte_array[8]   = {0};
    uint64_t key            = (uint64_t)sn;
    uint64_t out[2]         = {0};
    uint64_t h1             = 0;
    uint64_t h2             = 0;
    uint64_t combine        = 0;
    uint64_t bit_size       = 0;
    int hash_num            = 0;
    int bits_changed        = 0;
    int res                 = 0;

    if (NULL == bf) {
        return 0;
    }

    if (NULL == bf->hash_func) {
        return 0;
    }

    bit_size = bf->bitset->length * 64;
    hash_num = bf->bitset->hash_num;

    //little endian
    for (int i = 0; i < 8; i++) {
        *(byte_array + i) = (uint8_t)(key >> (i * 8));
    }

    bf->hash_func(byte_array, 8, bf->seed , out);

    h1 = *out;
    h2 = *(out + 1);

    combine = h1;

    for (int i = 0; i < hash_num; i++) {
        uint64_t bit_index = (combine & INT64_MAX) % bit_size;
        res = BitsSet(bf, bit_index);
        bits_changed |= res;
        combine += h2;
    }

    return bits_changed;
}

int MightContainStrNumber(BloomFilter *bf, StrNumber sn)
{
    uint8_t byte_array[8]   = {0};
    uint64_t key            = (uint64_t)atoll((const char *)sn.str);
    uint64_t out[2]         = {0};
    uint64_t h1             = 0;
    uint64_t h2             = 0;
    uint64_t combine        = 0;
    uint64_t *data          = NULL;
    uint64_t bit_size       = 0;
    int hash_num            = 0;

    if (NULL == bf) {
        return 0;
    }

    if (NULL == bf->bitset) {
        return 0;
    }

    if (NULL == bf->hash_func) {
        return 0;
    }

    if (bf->bitset->hash_num == 0) {
        return 0;
    }

    if (bf->bitset->length < 10) {
        return 0;
    }

    bit_size = bf->bitset->length * 64;
    hash_num = bf->bitset->hash_num;
    data = (uint64_t *)((void *)bf->bitset + HEADER_LEN);

    //little endian
    for (int i = 0; i < 8; i++) {
        *(byte_array + i) = (uint8_t)(key >> (i * 8));
    }

    bf->hash_func(byte_array, 8, bf->seed , out);
    h1 = *out;
    h2 = *(out + 1);

    //printf("key:%lu, h1:%lu, h2:%lu, num_hash:%d,INT64_MAX:%lu\n", key, h1, h2, bf->bitset->hash_num, INT64_MAX);

    combine = h1;

    for (int i = 0; i < hash_num; i++) {
        uint64_t bit_index = (combine & INT64_MAX) % bit_size;

        if (!BitsGet(data, bit_index)) {
            return 0;
        }

        combine = combine + h2;
    }

    return 1;
}

int MightContainNumber(BloomFilter *bf, double sn)
{
    uint8_t byte_array[8]   = {0};
    uint64_t key            = (uint64_t)sn;
    uint64_t out[2]         = {0};
    uint64_t h1             = 0;
    uint64_t h2             = 0;
    uint64_t combine        = 0;
    uint64_t *data          = NULL;
    uint64_t bit_size       = 0;
    int hash_num            = 0;

    if (NULL == bf) {
        return 0;
    }

    if (NULL == bf->bitset) {
        return 0;
    }

    if (NULL == bf->hash_func) {
        return 0;
    }

    if (bf->bitset->hash_num == 0) {
        return 0;
    }

    if (bf->bitset->length < 10) {
        return 0;
    }

    bit_size = bf->bitset->length * 64;
    hash_num = bf->bitset->hash_num;
    data = (uint64_t *)((void *)bf->bitset + HEADER_LEN);

    //little endian
    for (int i = 0; i < 8; i++) {
        *(byte_array + i) = (uint8_t)(key >> (i * 8));
    }

    bf->hash_func(byte_array, 8, bf->seed , out);
    h1 = *out;
    h2 = *(out + 1);

    //printf("key:%lu, h1:%lu, h2:%lu, num_hash:%d,INT64_MAX:%lu\n", key, h1, h2, bf->bitset->hash_num, INT64_MAX);

    combine = h1;

    for (int i = 0; i < hash_num; i++) {
        uint64_t bit_index = (combine & INT64_MAX) % bit_size;

        if (!BitsGet(data, bit_index)) {
            return 0;
        }

        combine = combine + h2;
    }

    return 1;
}

