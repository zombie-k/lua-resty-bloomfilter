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
