## 安装

```
git clone https://github.com/Xiangqian5/lua-resty-bloomfilter.git

cd lua-resty-bloomfilter

sh build.sh
```


## 使用

##### example.lua

```
local bloomfilter = require("bloomfilter")
local str_format = string.format
local ngx = ngx

if not bloomfilter.initted() then
    local initted, err = bloomfilter.init()
    if initted == nil then
        ngx.log(ngx.ERR, err)
        return
    end
end

local bf = bloomfilter.new_bf(1200, 0.001)

local element = 4501310677070684
local is_changed, err = bloomfilter.put_uint64(bf, element)
if is_changed == nil then
    ngx.say(err)
    return
end

local length = bf.bitset.length
ngx.say(str_format("bitset.length=%s", length))

--you may want to store the serialized result to redis.
local bitset, err = bloomfilter.serialized(bf)
if bitset == nil then
    ngx.say(err)
    return
end

local buf = ""
buf = buf .. "["
for i=0, length*8+6-1, 1 do
    buf = buf .. str_format("%s ", bitset[i])
end
buf = buf:sub(1,-2) .. "]"
ngx.say(buf)

local bf, err = bloomfilter.load_bf(bitset, length*8+6)
if bf == nil then
    ngx.say(err)
    return
end

local ele = "4501310677070684"
local is_in, err = bloomfilter.might_contain_str_number(bf, ele)
if is_in == nil then
    ngx.say(err)
    return
end
ngx.say(str_format("is_in:%s", is_in))

local ele2 = 4501310677070684
is_in, err = bloomfilter.might_contain_number(bf, ele2)
if is_in == nil then
    ngx.say(err)
    return
end
ngx.say(str_format("is_in:%s", is_in))

```



## 性能比较


```
BloomFilter info:
seed:0
num_hash:10
bit_array_size:17280ULL
bit_count:10ULL
hash_func:cdata<void (*)()>: 0x7feb7adc7ff0

Api MightContainStrNumber is_in:1  loop:1          cost:0.010967254638672
Api MightContainNumber    is_in:1  loop:1          cost:0.0050067901611328
Api MightContainStrNumber is_in:1  loop:1000       cost:0.92315673828125
Api MightContainNumber    is_in:1  loop:1000       cost:0.18405914306641
Api MightContainStrNumber is_in:1  loop:3000       cost:2.2499561309814
Api MightContainNumber    is_in:1  loop:3000       cost:0.41890144348145
Api MightContainStrNumber is_in:1  loop:5000       cost:3.5860538482666
Api MightContainNumber    is_in:1  loop:5000       cost:0.7469654083252
Api MightContainStrNumber is_in:1  loop:10000      cost:7.2948932647705
Api MightContainNumber    is_in:1  loop:10000      cost:1.3251304626465
Api MightContainStrNumber is_in:1  loop:100000     cost:64.152956008911
Api MightContainNumber    is_in:1  loop:100000     cost:13.160943984985
Api MightContainStrNumber is_in:1  loop:1000000    cost:639.33396339417
Api MightContainNumber    is_in:1  loop:1000000    cost:130.28979301453
```
