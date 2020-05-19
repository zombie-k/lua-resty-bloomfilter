//
// Created by xiangqian5 on 2020/5/13.
//

#ifndef LUA_RESTY_BLOOMFILTER_MURMURHASH3_H
#define LUA_RESTY_BLOOMFILTER_MURMURHASH3_H

#if defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))

#include <cxxabi.h>

#endif

#include <stdint.h>

uint32_t MurmurHash3_x86_32  ( const void * key, int len, uint32_t seed);
void MurmurHash3_x86_128 ( const void * key, int len, uint32_t seed, void * out );
void MurmurHash3_x64_128 ( const void * key, int len, uint32_t seed, void * out );

#endif //LUA_RESTY_BLOOMFILTER_MURMURHASH3_H
