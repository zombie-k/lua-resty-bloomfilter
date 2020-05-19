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

#ifndef BLOOMFILTER_BLOOMFILTER_H
#define BLOOMFILTER_BLOOMFILTER_H

#if defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))
#include <cxxabi.h>
#else
#include <stdint.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../murmurhash3/murmurhash3.h"

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#define BF_NTOHL(x) \
        ((uint32_t)((((uint32_t)(x) & 0xff000000) >> 24) | \
	        (((uint32_t)(x) & 0x00ff0000) >>  8) | \
	        (((uint32_t)(x) & 0x0000ff00) <<  8) | \
	        (((uint32_t)(x) & 0x000000ff) << 24)))

#define BF_HTONL(x) \
        ((uint32_t)((((uint32_t)(x) & 0xff000000) >> 24) | \
	        (((uint32_t)(x) & 0x00ff0000) >>  8) | \
	        (((uint32_t)(x) & 0x0000ff00) <<  8) | \
	        (((uint32_t)(x) & 0x000000ff) << 24)))

#define BF_NTOHLL(x) \
     ((uint64_t)((((uint64_t)(x) & 0xff00000000000000ULL) >> 56) | \
	        (((uint64_t)(x) & 0x00ff000000000000ULL) >> 40) | \
	        (((uint64_t)(x) & 0x0000ff0000000000ULL) >> 24) | \
	        (((uint64_t)(x) & 0x000000ff00000000ULL) >>  8) | \
	        (((uint64_t)(x) & 0x00000000ff000000ULL) <<  8) | \
	        (((uint64_t)(x) & 0x0000000000ff0000ULL) << 24) | \
	        (((uint64_t)(x) & 0x000000000000ff00ULL) << 40) | \
	        (((uint64_t)(x) & 0x00000000000000ffULL) << 56)))

#define BF_HTONLL(x) \
     ((uint64_t)((((uint64_t)(x) & 0xff00000000000000ULL) >> 56) | \
	        (((uint64_t)(x) & 0x00ff000000000000ULL) >> 40) | \
	        (((uint64_t)(x) & 0x0000ff0000000000ULL) >> 24) | \
	        (((uint64_t)(x) & 0x000000ff00000000ULL) >>  8) | \
	        (((uint64_t)(x) & 0x00000000ff000000ULL) <<  8) | \
	        (((uint64_t)(x) & 0x0000000000ff0000ULL) << 24) | \
	        (((uint64_t)(x) & 0x000000000000ff00ULL) << 40) | \
	        (((uint64_t)(x) & 0x00000000000000ffULL) << 56)))

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#define BF_NTOHL(x)        (x)
#define BF_NTOHLL(x)       (x)
#define BF_HTONL(x)        (x)
#define BF_HTONLL(x)       (x)

#else
#error unspecified endianness
#endif


typedef void (*HashFunc)(const void * key, const int len, uint32_t seed, void* out);

typedef struct {
    //string
    uint8_t str[20];

    //string width
    uint8_t width;

    //reverse
    uint8_t reverse[3];
} StrNumber;

#pragma pack(2)
typedef struct {
    int8_t  magic;

    //Number of hash functions.
    uint8_t hash_num;

    //1 big endian int, the number of longs in the bitset.
    uint32_t length;

    //N big endian longs of the bitset behind.
} BitSetHeader;
#pragma unpack()

#define HEADER_LEN      (sizeof(BitSetHeader))

typedef struct {
    //seed
    uint32_t seed;

    //
    uint64_t bit_count;

    //hash callback
    HashFunc hash_func;

    //bitset
    BitSetHeader *bitset;
} BloomFilter;


/*
 *  API.
 */


/*
 * @Description : Calculate the optimal number of bloom bits.
 * @Date        : 2020-05-15
 * @Author      : Xiangqian5
 * @Software    : Weibo Inc.
 *
 * @param:
 *  n           : The number of elements.
 *  p           : The rate of positive.
 *
 * @return:
 *  m           : Bloom's bits num.
 */

uint64_t OptimalNumOfBits(uint64_t n, double p);

/*
 * @Description : Calculate the optimal number of hash.
 * @Date        : 2020-05-15
 * @Author      : Xiangqian5
 * @Software    : Weibo Inc.
 *
 * @param:
 *  n           : The number of elements.
 *  m           : The size of bloom bits.
 *
 * @return:
 * k            : Number of hash.
 */

int OptimalNumOfHash(uint64_t n, uint64_t m);

/*
 * @Description : Load data from redis and create a bloom filter
 * @Date        : 2020-05-15
 * @Author      : Xiangqian5
 * @Software    : Weibo Inc.
 *
 * @param
 *  byte_array  : A byte array from redis.
 *  array_len   : length of byte array.
 *
 * @return
 *  bf          : A bloom filter struct ptr.
 */

BloomFilter *LoadBF(void *byte_array, double array_len);

/*
 * @Description : New a bloom filter instance.
 * @Date        : 2020-05-15
 * @Author      : Xiangqian5
 * @Software    : Weibo Inc.
 *
 * @param:
 *  expect      : number of bloom elements.
 *  fpp         : positive percent.
 *
 * @return:
 *  bf          : pointer of bloom filter.
 */

BloomFilter *NewBF(uint64_t expect, double fpp);

/*
 * @Description : Destroy a bloom filter.
 * @Date        : 2020-05-15
 * @Author      : Xiangqian5
 * @Software    : Weibo Inc.
 *
 * @param
 *  bf          : The bloom filter to destroy.
 *
 * @return
 *              : void.
 */

void DestroyBF(BloomFilter *bf);

/*
 * @Description : Serialize bloom data.
 * @Date        : 2020-05-15
 * @Author      : Xiangqian5
 * @Software    : Weibo Inc.
 *
 * @param:
 *  bf          : The bloom filter to storage.
 *
 * @return:
 *  bytes_array : A big endian byte array.
 */

uint8_t * Serialized(BloomFilter *bf);

/*
 * @Description : Check element is in the bloom or not.
 * @Date        : 2020-05-15
 * @Author      : Xiangqian5
 * @Software    : Weibo Inc.
 *
 * @param
 *  bf          : The bloom filter .
 *  sn          : The element to check.
 *
 * @return
 *  is_in       : 0->not in. 1->in.
 */

int MightContainStrNumber(BloomFilter *bf, StrNumber sn);

/*
 * @Description : Check number element is in the bloom or not.
 * @Date        : 2020-05-15
 * @Author      : Xiangqian5
 * @Software    : Weibo Inc.
 *
 * @param
 *  bf          : The bloom filter .
 *  sn          : The element to check.
 *
 * @return
 *  is_in       : 0->not in. 1->in.
 */

int MightContainNumber(BloomFilter *bf, double sn);

/*
 * @Description : Put a StrNumber element into bloom.
 * @Date        : 2020-05-15
 * @Author      : Xiangqian5
 * @Software    : Weibo Inc.
 *
 * @param:
 *  bf          : The bloom filter .
 *  sn          : The element to put.
 *
 * @return:
 *  ok          : 0->fail. 1->ok.
 */

int PutStrNumber(BloomFilter *bf, StrNumber sn);

/*
 * @Description : Put a double element into bloom.
 * @Date        : 2020-05-15
 * @Author      : Xiangqian5
 * @Software    : Weibo Inc.
 *
 * @param:
 *  bf          : The bloom filter .
 *  sn          : The element to put.
 *
 * @return:
 *  ok          : 0->fail. 1->ok.
 */

int PutUint64(BloomFilter *bf, double sn);

#endif //BLOOMFILTER_BLOOMFILTER_H
