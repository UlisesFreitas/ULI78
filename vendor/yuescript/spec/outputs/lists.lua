local hi
do
	local _accum_0 = { }
	local _len_0 = 1
	for _, x in ipairs({
		1,
		2,
		3,
		4
	}) do
		_accum_0[_len_0] = x * 2
		_len_0 = _len_0 + 1
	end
	hi = _accum_0
end
local items = {
	1,
	2,
	3,
	4,
	5,
	6
}
local _
do
	local _accum_0 = { }
	local _len_0 = 1
	for z in ipairs(items) do
		if z > 4 then
			_accum_0[_len_0] = z
			_len_0 = _len_0 + 1
		end
	end
	_ = _accum_0
end
local rad
do
	local _accum_0 = { }
	local _len_0 = 1
	for a in ipairs({
		1,
		2,
		3,
		4,
		5,
		6
	}) do
		if good_number(a) then
			_accum_0[_len_0] = {
				a
			}
			_len_0 = _len_0 + 1
		end
	end
	rad = _accum_0
end
do
	local _accum_0 = { }
	local _len_0 = 1
	for z in items do
		for j in list do
			if z > 4 then
				_accum_0[_len_0] = z
				_len_0 = _len_0 + 1
			end
		end
	end
	_ = _accum_0
end
require("util")
local dump
dump = function(x)
	return print(util.dump(x))
end
local range
range = function(count)
	local i = 0
	return coroutine.wrap(function()
		while i < count do
			coroutine.yield(i)
			i = i + 1
		end
	end)
end
dump((function()
	local _accum_0 = { }
	local _len_0 = 1
	for x in range(10) do
		_accum_0[_len_0] = x
		_len_0 = _len_0 + 1
	end
	return _accum_0
end)())
dump((function()
	local _accum_0 = { }
	local _len_0 = 1
	for x in range(5) do
		if x > 2 then
			for y in range(5) do
				_accum_0[_len_0] = {
					x,
					y
				}
				_len_0 = _len_0 + 1
			end
		end
	end
	return _accum_0
end)())
local things
do
	local _accum_0 = { }
	local _len_0 = 1
	for x in range(10) do
		if x > 5 then
			for y in range(10) do
				if y > 7 then
					_accum_0[_len_0] = x + y
					_len_0 = _len_0 + 1
				end
			end
		end
	end
	things = _accum_0
end
for x in ipairs({
	1,
	2,
	4
}) do
	for y in ipairs({
		1,
		2,
		3
	}) do
		if x ~= 2 then
			print(x, y)
		end
	end
end
for x in items do
	print("hello", x)
end
do
	local _accum_0 = { }
	local _len_0 = 1
	for x in x do
		_accum_0[_len_0] = x
		_len_0 = _len_0 + 1
	end
	_ = _accum_0
end
local x
do
	local _accum_0 = { }
	local _len_0 = 1
	for x in x do
		_accum_0[_len_0] = x
		_len_0 = _len_0 + 1
	end
	x = _accum_0
end
for x in ipairs({
	1,
	2,
	4
}) do
	for y in ipairs({
		1,
		2,
		3
	}) do
		if x ~= 2 then
			print(x, y)
		end
	end
end
local double
do
	local _accum_0 = { }
	local _len_0 = 1
	for _index_0 = 1, #items do
		local x = items[_index_0]
		_accum_0[_len_0] = x * 2
		_len_0 = _len_0 + 1
	end
	double = _accum_0
end
for _index_0 = 1, #double do
	local x = double[_index_0]
	print(x)
end
local cut
do
	local _accum_0 = { }
	local _len_0 = 1
	for _index_0 = 1, #items do
		local x = items[_index_0]
		if x > 3 then
			_accum_0[_len_0] = x
			_len_0 = _len_0 + 1
		end
	end
	cut = _accum_0
end
local hello
do
	local _accum_0 = { }
	local _len_0 = 1
	for _index_0 = 1, #items do
		local x = items[_index_0]
		for _index_1 = 1, #items do
			local y = items[_index_1]
			_accum_0[_len_0] = x + y
			_len_0 = _len_0 + 1
		end
	end
	hello = _accum_0
end
for _index_0 = 1, #hello do
	local z = hello[_index_0]
	print(z)
end
x = {
	1,
	2,
	3,
	4,
	5,
	6,
	7
}
local _max_0 = #x + -5 + 1
for _index_0 = 2, _max_0, 2 do
	local y = x[_index_0]
	print(y)
end
for _index_0 = 1, 3 do
	local y = x[_index_0]
	print(y)
end
local _max_1 = #x
for _index_0 = 2, _max_1 do
	local y = x[_index_0]
	print(y)
end
local _max_2 = #x
for _index_0 = 1, _max_2, 2 do
	local y = x[_index_0]
	print(y)
end
local _max_3 = #x
for _index_0 = 2, _max_3, 2 do
	local y = x[_index_0]
	print(y)
end
local a, b, c = 1, 5, 2
local _min_0 = a
local _max_4 = b
_min_0 = _min_0 < 0 and #x + _min_0 + 1 or _min_0
_max_4 = _max_4 < 0 and #x + _max_4 + 1 or _max_4
for _index_0 = _min_0, _max_4, c do
	local y = x[_index_0]
	print(y)
end
local normal
normal = function(hello)
	local _accum_0 = { }
	local _len_0 = 1
	for x in yeah do
		_accum_0[_len_0] = x
		_len_0 = _len_0 + 1
	end
	return _accum_0
end
local test = x(1, 2, 3, 4, 5)
for _index_0 = 1, #test do
	local thing = test[_index_0]
	print(thing)
end
_ = function()
	local _list_0 = rows
	for _index_0 = 1, #_list_0 do
		local row = _list_0[_index_0]
		a = b
	end
end
do
	local _with_0 = tb
	f(_with_0[a])
	f({
		a
	})
	local _list_0 = f
	local _min_1 = a
	local _max_5 = #_list_0
	_min_1 = _min_1 < 0 and #_list_0 + _min_1 + 1 or _min_1
	for _index_0 = _min_1, _max_5 do
		local v = _list_0[_index_0]
		print(v)
	end
	f({ })
	do
		local _obj_0 = f
		_obj_0[#_obj_0 + 1] = x
	end
	_with_0[a] = x
	a = x[1]
	_with_0[#_with_0 + 1] = x
	do
		local _check_0 = _with_0[1]
		local _find_0 = false
		for _index_0 = 1, #_check_0 do
			local _item_0 = _check_0[_index_0]
			if _item_0 == a then
				_find_0 = true
				break
			end
		end
		c = _find_0
	end
	c = (1 == a)
	c = (1 == a)
	c = (1 == a)
end
do
	a, b = hello[1], hello[2]
	local name, job
	local _obj_0 = person
	name, job = _obj_0[1], _obj_0[2]
	if name == nil then
		name = "nameless"
	end
	if job == nil then
		job = "jobless"
	end
end
do
	local transactions = {
		"T001",
		"T002",
		"T003",
		"T004",
		"T005"
	}
	local middleTransactions
	do
		local _accum_0 = { }
		local _len_0 = 1
		local _max_5 = #transactions + -2 + 1
		for _index_0 = 2, _max_5 do
			local _item_0 = transactions[_index_0]
			_accum_0[_len_0] = _item_0
			_len_0 = _len_0 + 1
		end
		middleTransactions = _accum_0
	end
	print(middleTransactions)
end
do
	local logs = {
		{
			start = 0,
			["end"] = 100
		},
		{
			start = 100,
			["end"] = 200
		},
		{
			start = 200,
			["end"] = 123
		}
	}
	print(logs[#logs]["end"])
end
do
	local pendingOrders = {
		"O001",
		"O002",
		"O003",
		"O004"
	}
	print(pendingOrders[#pendingOrders - 1])
end
do
	local getOrders
	getOrders = function()
		return {
			{
				id = "O1001",
				status = "pending"
			},
			{
				id = "O1002",
				status = "processing"
			},
			{
				id = "O1003",
				status = "done"
			}
		}
	end
	local lastStatus
	do
		local _item_0 = getOrders()
		lastStatus = _item_0[#_item_0].status
	end
	assert(lastStatus == "done")
end
do
	local cloneList1
	cloneList1 = function(list)
		local _accum_0 = { }
		local _len_0 = 1
		local _max_5 = #list
		for _index_0 = 1, _max_5 do
			local _item_0 = list[_index_0]
			_accum_0[_len_0] = _item_0
			_len_0 = _len_0 + 1
		end
		return _accum_0
	end
	local cloneList2
	cloneList2 = function(list)
		local _tab_0 = { }
		local _idx_0 = #_tab_0 + 1
		for _index_0 = 1, #list do
			local _value_0 = list[_index_0]
			_tab_0[_idx_0] = _value_0
			_idx_0 = _idx_0 + 1
		end
		return _tab_0
	end
	local cloneTable
	cloneTable = function(tb)
		local _tab_0 = { }
		local _idx_0 = 1
		for _key_0, _value_0 in pairs(tb) do
			if _idx_0 == _key_0 then
				_tab_0[#_tab_0 + 1] = _value_0
				_idx_0 = _idx_0 + 1
			else
				_tab_0[_key_0] = _value_0
			end
		end
		return _tab_0
	end
end
do
	print((function()
		local _item_0 = globalTB
		return _item_0[#_item_0]
	end)(), (function()
		local _item_0 = a.b.c
		return _item_0[#_item_0 - 2]
	end)(), (function()
		if x ~= nil then
			local _obj_0 = x.y
			if _obj_0 ~= nil then
				local _obj_1 = _obj_0(x).z
				if _obj_1 ~= nil then
					return _obj_1[#_obj_1 - 3]
				end
				return nil
			end
			return nil
		end
		return nil
	end)())
end
local _anon_func_0 = function(globalTB)
	local _call_0
	do
		local _item_0 = globalTB
		_call_0 = _item_0[#_item_0]
	end
	return _call_0["end"](_call_0, 123)
end
local _anon_func_1 = function(a)
	local _item_0
	do
		local _accum_0 = { }
		local _len_0 = 1
		local _list_0 = a.b.c
		local _max_5 = #_list_0 + -5 + 1
		for _index_0 = 5, _max_5 do
			local _item_1 = _list_0[_index_0]
			_accum_0[_len_0] = _item_1
			_len_0 = _len_0 + 1
		end
		_item_0 = _accum_0
	end
	return _item_0[#_item_0 - 2]
end
local _anon_func_2 = function(x)
	if x ~= nil then
		local _obj_0 = x.y
		if _obj_0 ~= nil then
			local _obj_1 = _obj_0(x).z
			if _obj_1 ~= nil then
				local _obj_2 = _obj_1[#_obj_1 - 3]
				if _obj_2 ~= nil then
					local _accum_0 = { }
					local _len_0 = 1
					local _max_5 = #_obj_2 + -3 + 1
					for _index_0 = 1, _max_5 do
						local _item_0 = _obj_2[_index_0]
						_accum_0[_len_0] = _item_0
						_len_0 = _len_0 + 1
					end
					return _accum_0
				end
				return nil
			end
			return nil
		end
		return nil
	end
	return nil
end
do
	local f
	f = function()
		return print(_anon_func_0(globalTB), _anon_func_1(a), _anon_func_2(x))
	end
end
do
	local tb = {
		1,
		2,
		3
	}
	tb[#tb] = 40
	tb[#tb - 1] = 20
end
do
	a = "x"
	b = a
	c = a
	local lst = { }
	lst[#lst] = a
	lst[#lst - 1] = b
end
do
	local y, z
	x, y, z = 1, 2, 3
	local arr = { }
	local head
	arr[#arr], head = x, y
	arr[#arr] = z
end
do
	local triple = {
		"keep",
		"skip",
		"tail"
	}
	local head, tailv = triple[1], triple[3]
	local buf = { }
	buf[#buf] = head
	buf[#buf] = tailv
end
do
	local src = {
		"a",
		"",
		"c",
		nil,
		"d"
	}
	local collected = { }
	for _index_0 = 1, #src do
		local item = src[_index_0]
		if item and #item > 0 then
			collected[#collected] = item
		end
	end
end
do
	local nums = {
		1,
		2,
		3,
		4,
		5
	}
	local last_two
	do
		local _accum_0 = { }
		local _len_0 = 1
		for _index_0 = 1, #nums do
			local v = nums[_index_0]
			if v > 3 then
				_accum_0[_len_0] = v
				_len_0 = _len_0 + 1
			end
		end
		last_two = _accum_0
	end
	nums[#nums] = last_two[1]
	nums[#nums] = last_two[2]
end
do
	local store = { }
	store[#store] = {
		meta = {
			id = 1,
			ok = true
		},
		payload = {
			10,
			20
		}
	}
	store[#store] = {
		meta = {
			id = 1,
			ok = false
		},
		payload = {
			10,
			20,
			30
		}
	}
end
local _anon_func_3 = function(tb)
	local _item_0 = tb.tmp
	return _item_0[#_item_0]
end
do
	local f
	f = function()
		local q = { }
		do
			local _accum_0 = { }
			local _len_0 = 1
			for n = 1, 4 do
				_accum_0[_len_0] = n
				_len_0 = _len_0 + 1
			end
			tb.tmp = _accum_0
		end
		if #tb.tmp >= 3 then
			q[#q] = {
				head = tb.tmp[1],
				tail = _anon_func_3(tb)
			}
		end
	end
end
do
	local make_pair
	make_pair = function(a, b)
		return {
			a,
			b
		}
	end
	local pairs = { }
	local p1 = make_pair(7, 8)
	pairs[#pairs] = p1
	local k, v = "key", 42
	pairs[#pairs] = {
		k = k,
		v = v
	}
end
do
	local cfg = {
		mode = "safe",
		tags = { }
	}
	if cfg.mode == "safe" then
		cfg.mode = "fast"
		local _obj_0 = cfg.tags
		_obj_0[#_obj_0] = "newbie"
	end
end
do
	local mat = {
		{
			1,
			2
		},
		{
			3,
			4
		},
		{
			5,
			6
		}
	}
	local last_row = mat[#mat]
	local rows = { }
	rows[#rows] = last_row[1]
end
do
	local kv = { }
	kv[#kv] = {
		k = "a",
		v = 1
	}
	kv[#kv] = {
		k = "b",
		v = 2
	}
	local pair_last = kv[#kv]
	local dict = { }
	dict[pair_last.k] = pair_last.v
	dict[pair_last.k] = 3
end
do
	local base
	do
		local _accum_0 = { }
		local _len_0 = 1
		for i = 1, 4 do
			_accum_0[_len_0] = i
			_len_0 = _len_0 + 1
		end
		base = _accum_0
	end
	local pack = { }
	pack[#pack] = {
		base[1],
		base[#base]
	}
	pack[#pack] = {
		first = base[1],
		last = base[#base]
	}
end
do
	local opts = {
		limit = 10
	}
	local limit, offset = opts.limit, opts.offset
	if offset == nil then
		offset = 0
	end
	local pages = { }
	pages[#pages] = {
		limit = limit,
		offset = offset
	}
end
do
	local chain = {
		a = {
			b = {
				c = 0
			}
		},
		list = {
			{
				x = 0
			},
			{
				x = 0
			}
		}
	}
	chain.a.b.c = 1
	chain.list[1].x = 10;
	((function()
		local _item_0 = chain.list
		return _item_0[#_item_0]
	end)()).x = 20
	local _obj_0 = chain.list
	_obj_0[#_obj_0 - 1] = {
		x = 30
	}
end
do
	local node = {
		left = {
			v = 0
		},
		right = {
			v = 0
		}
	}
	local bag = { }
	local left, right = node.left, node.right
	bag[#bag], left.v, right.v = "k", 1, 2
end
do
	local a1, a2, a3 = 100, 200, 300
	local mix = { }
	local meta
	mix[#mix], mix[#mix], meta = a1, a2, {
		tag = "ok"
	}
end
do
	local cfg2 = {
		limit = 5,
		opts = {
			flag = false
		}
	}
	local lim, opt2 = cfg2.limit, cfg2.opts
	local bucket = {
		xs = { }
	}
	local _obj_0 = bucket.xs
	_obj_0[#_obj_0] = lim
	bucket.flag = true
	local _obj_1 = opt2.flags
	_obj_1[#_obj_1 + 1] = 123
end
do
	local ret2
	ret2 = function()
		return 7, 8
	end
	local box = { }
	local x1
	box[#box], x1 = ret2()
end
do
	local q = {
		1,
		2
	}
	local lastq = q[#q]
	q[#q - 1] = lastq * 10
end
do
	local mat2 = [[9,8], [7,6]]
	local t = {
		hold = nil
	}
	t.hold = mat2[#mat2][1]
end
do
	local f
	f = function()
		local _obj_0
		do
			local _item_0 = globalTB
			_obj_0 = _item_0[#_item_0]
		end
		_obj_0[#_obj_0] = 1
	end
	local f1
	f1 = function()
		do
			local _item_0 = globalTB
			do
				local _item_1 = _item_0[#_item_0]
				return _item_1[#_item_1 - 1]
			end
		end
	end
end
return nil
