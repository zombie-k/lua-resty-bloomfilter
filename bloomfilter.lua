local ffi = require("ffi")

local ffi_gc = ffi.gc
local ffi_typeof = ffi.typeof
local new_tab = table.new
local str_gmatch = string.gmatch
local str_match = string.match
local io_open = io.open
local io_close = io.close
local cpath = package.cpath
local str_format = string.format
--local load_shared_lib = load_shared_lib
local pcall = pcall

ffi.cdef[[
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

BloomFilter *LoadBF(void *byte_array, double array_len);
void DestroyBF(BloomFilter *bf);

BloomFilter *NewBF(uint64_t expect, double fpp);
uint8_t * Serialized(BloomFilter *bf);

int MightContainNumber(BloomFilter *bf, double sn);
int MightContainStrNumber(BloomFilter *bf, StrNumber sn);

int PutUint64(BloomFilter *bf, double sn);

]]

local function load_shared_lib(lib_name)
    local tried_paths = new_tab(32, 0)
    local i = 1

    for k, _ in str_gmatch(cpath, "[^;]+") do
        local dir = str_match(k, "(.*/)")
        local fpath = dir .. lib_name
        local f = io_open(fpath)
        if f ~= nil then
            io_close(f)
            tried_paths[i] = dir
            return ffi.load(fpath), tried_paths
        end
        tried_paths[i] = fpath
        i = i + 1
    end

    return nil, tried_paths
end

local _M = {}

local StrNumber = ffi_typeof('StrNumber')
local initted = false
local handler

function _M.initted()

    return initted
end

function _M.init()
    handler = load_shared_lib("libbloomfilter.so")

    if handler == nil then
        return nil, "aborted load libbloomfilter.so error."
    end

    initted = true

    return initted, nil
end

function _M.new_bf(expect, fpp)
    local ok, bf = pcall(handler.NewBF, expect, fpp)
    if not ok then
        return nil, str_format("aborted new bloomfilter error. %s", bf)
    end

    bf = ffi_gc(bf, handler.DestroyBF)

    return bf, nil
end

function _M.load_bf(byte_array, array_len)
    local ok, bf = pcall(handler.LoadBF, byte_array, array_len)
    if not ok then
        return nil, str_format("aborted load bf error. %s", bf)
    end

    bf = ffi_gc(bf, handler.DestroyBF)

    return bf, nil
end

function _M.might_contain_str_number(bf, element)
    local str_number = StrNumber {str = element}
    local ok, is_in = pcall(handler.MightContainStrNumber, bf, str_number)
    if not ok then
        return nil, str_format("aborted might_contain_str_number error. %s", is_in)
    end

    return is_in, nil
end

function _M.might_contain_number(bf, element)
    local ok, is_in = pcall(handler.MightContainNumber, bf, element)
    if not ok then
        return nil, str_format("aborted might_contain_number error. %s", is_in)
    end

    return is_in, nil
end

function _M.put_uint64(bf, element)
    local ok, is_changed = pcall(handler.PutUint64, bf, element)
    if not ok then
        return nil, str_format("aborted put uint64 error. %s", is_changed)
    end

    return is_changed, nil
end

function _M.serialized(bf)
    local ok, bitset = pcall(handler.Serialized, bf)
    if not ok then
        return nil, str_format("aborted serialized error. %s", bitset)
    end

    return bitset, nil
end

return _M
