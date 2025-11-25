local x
x = function()
	return print(what)
end
local _
_ = function() end
_ = function()
	return function()
		return function() end
	end
end
go(to(the(barn)))
open(function()
	return the(function()
		return door
	end)
end)
open(function()
	the(door)
	local hello
	hello = function()
		return my(func)
	end
end)
local h
h = function()
	return hi
end
eat(function() end, world);
(function() end)()
x = function(...) end
hello()
hello.world()
_ = hello().something
_ = what()["ofefe"]
what()(the()(heck()))
_ = function(a, b, c, d, e) end
_ = function(a, a, a, a, a)
	return print(a)
end
_ = function(x)
	if x == nil then
		x = 23023
	end
end
_ = function(x)
	if x == nil then
		x = function(y)
			if y == nil then
				y = function() end
			end
		end
	end
end
_ = function(x)
	if x == nil then
		if something then
			x = yeah
		else
			x = no
		end
	end
end
local something
something = function(hello, world)
	if hello == nil then
		hello = 100
	end
	if world == nil then
		world = function(x)
			if x == nil then
				x = [[yeah cool]]
			end
			return print("eat rice")
		end
	end
	return print(hello)
end
_ = function(self) end
_ = function(self, x, y) end
_ = function(self, x, y)
	self.x = x
	self.y = y
end
_ = function(self, x)
	if x == nil then
		x = 1
	end
end
_ = function(self, x, y, z)
	if x == nil then
		x = 1
	end
	if z == nil then
		z = "hello world"
	end
	self.x = x
	self.z = z
end
x(function()
	return
end)
y(function()
	return 1
end)
z(function()
	return 1, "hello", "world"
end)
k(function()
	if yes then
		return
	else
		return
	end
end)
_ = function()
	if something then
		return real_name
	end
end
d(function()
	return print("hello world")
end, 10)
d(1, 2, 3, 4, 5, 6, (function()
	if something then
		print("okay")
		return 10
	end
end)(), 10, 20)
f()()(what)(function()
	return print("srue")
end, 123)
x = function(a, b)
	return print("what")
end
local y
y = function(a, b)
	if a == nil then
		a = "hi"
	end
	if b == nil then
		b = 23
	end
	return print("what")
end
local z
z = function(a, b)
	if a == nil then
		a = "hi"
	end
	if b == nil then
		b = 23
	end
	return print("what")
end
local j
j = function(f, g, m, a, b)
	if a == nil then
		a = "hi"
	end
	if b == nil then
		b = 23
	end
	return print("what")
end
y = function(a, b, ...)
	if a == nil then
		a = "hi"
	end
	if b == nil then
		b = 23
	end
	return print("what")
end
y = function(a, b, ...)
	if a == nil then
		a = "hi"
	end
	if b == nil then
		b = 23
	end
	return print("what")
end
local args
args = function(a, b)
	return print("what")
end
args = function(a, b)
	if a == nil then
		a = "hi"
	end
	if b == nil then
		b = 23
	end
	return print("what")
end
args = function(a, b)
	if a == nil then
		a = "hi"
	end
	if b == nil then
		b = 23
	end
	return print("what")
end
args = function(f, g, m, a, b)
	if a == nil then
		a = "hi"
	end
	if b == nil then
		b = 23
	end
	return print("what")
end
local self
self = function(n)
	if n == 0 then
		return 1
	end
	return n * self(n - 1)
end
do
	items.every(function(item)
		if item.field then
			local value = item.field.get("abc")
			if value then
				local _exp_0 = value:get()
				if 123 == _exp_0 then
					return false
				elseif 456 == _exp_0 then
					handle(item)
				end
			end
		end
		return true
	end)
	items.every(function(item)
		if item.field then
			local value = item.field.get("abc")
			if value then
				local _exp_0 = value:get()
				if 123 == _exp_0 then
					return false
				elseif 456 == _exp_0 then
					handle(item)
				end
			end
		end
		return true
	end)
	HttpServer:post("/login", function(req)
		do
			local _type_0 = type(req)
			local _tab_0 = "table" == _type_0 or "userdata" == _type_0
			if _tab_0 then
				local name = req.name
				local pwd = req.pwd
				if name ~= nil and pwd ~= nil then
					if name ~= "" then
						local user = DB:queryUser(name, pwd)
						if user then
							if user.status == "available" then
								return {
									success = true
								}
							end
						end
					end
				end
			end
		end
		return {
			success = false
		}
	end)
	local check
	check = function(num)
		return num
	end
	local func
	func = function()
		check(123)
	end
	print(func())
end
local _anon_func_0 = function(_arg_0)
	local _accum_0 = { }
	local _len_0 = 1
	local _max_0 = #_arg_0
	for _index_0 = 1, _max_0 do
		local _item_0 = _arg_0[_index_0]
		_accum_0[_len_0] = _item_0
		_len_0 = _len_0 + 1
	end
	return _accum_0
end
do
	local f
	f = function(_arg_0)
		local a, b, c
		a, b, c = _arg_0.a, _arg_0.b, _arg_0.c
		return print(a, b, c)
	end
	f = function(_arg_0)
		local a, b, c
		a, b, c = _arg_0.a, _arg_0.b, _arg_0.c
		return print(a, b, c)
	end
	local g
	g = function(x, _arg_0)
		local y
		y = _arg_0.y
		return print(x, y)
	end
	local i
	i = function(_arg_0)
		local ax, by
		ax, by = _arg_0.a, _arg_0.b
		if ax == nil then
			ax = 0
		end
		if by == nil then
			by = 0
		end
		return print(ax, by)
	end
	j = function(name, _arg_0)
		local uid, role
		uid, role = _arg_0.id, _arg_0.role
		if uid == nil then
			uid = "n/a"
		end
		if role == nil then
			role = "guest"
		end
		return print(name, uid, role)
	end
	local m
	m = function(_arg_0)
		local name, age, ver
		name, age, ver = _arg_0.user.name, _arg_0.user.age, _arg_0.meta.ver
		if ver == nil then
			ver = 1
		end
		return print(name, age, ver)
	end
	local m1
	m1 = function(_arg_0)
		local name, age, meta
		name, age, meta = _arg_0.user.name, _arg_0.user.age, _arg_0.meta
		if meta == nil then
			meta = { }
		end
		return print(name, age, meta and meta.ver or "nil")
	end
	local new
	new = function(self, _arg_0)
		local name, age
		name, age = _arg_0.name, _arg_0.age
		if name == nil then
			name = "anon"
		end
		if age == nil then
			age = 0
		end
		self.name = name
		self.age = age
	end
	local set
	set = function(self, _arg_0)
		local name, age
		name, age = _arg_0.name, _arg_0.age
		if name == nil then
			name = self.name
		end
		if age == nil then
			age = self.age
		end
		self.name = name
		self.age = age
	end
	local logKV
	logKV = function(_arg_0, ...)
		local k, v
		k, v = _arg_0.k, _arg_0.v
		print("kv:", k, v)
		return print("rest count:", select("#", ...))
	end
	do
		local foo
		foo = function(_arg_0)
			local a, b
			a, b = _arg_0.a, _arg_0.b
			if b == nil then
				b = 0
			end
			return print(a, b)
		end
	end
	local t1
	t1 = function(_arg_0, x)
		local a
		a = _arg_0.a
		return print(a, x)
	end
	local t2
	t2 = function(_arg_0)
		local a
		a = _arg_0.a
		return print(a)
	end
	local w
	w = function(id, _arg_0, _arg_1)
		local x, y
		x, y = _arg_0.x, _arg_0.y
		if x == nil then
			x = 0
		end
		if y == nil then
			y = 0
		end
		local flag
		flag = _arg_1.flag
		return print(id, x, y, flag)
	end
	local g1
	g1 = function(_arg_0)
		local a, ax
		a, ax = _arg_0.a, _arg_0.a
		return print(a, ax)
	end
	local g4
	g4 = function(_arg_0)
		local a, b, rest
		a, b, rest = _arg_0.a, _arg_0.b, _anon_func_0(_arg_0)
		return print(a, b)
	end
end
return nil
