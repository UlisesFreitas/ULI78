local a, b, c, d = 1, 2, 3, 4
do
	local a, b = setmetatable({ }, {
		__close = function(self)
			return print("closed")
		end
	})
	local _close_0
	if (function()
		local _val_0 = type(a)
		return 'table' == _val_0 or 'userdata' == _val_0
	end)() then
		_close_0 = assert(getmetatable(a) and getmetatable(a).__close, "variable 'a' got a non-closable value")
	elseif a == nil then
		_close_0 = nil
	else
		_close_0 = error("variable 'a' got a non-closable value")
	end
	local _close_1
	if (function()
		local _val_0 = type(b)
		return 'table' == _val_0 or 'userdata' == _val_0
	end)() then
		_close_1 = assert(getmetatable(b) and getmetatable(b).__close, "variable 'b' got a non-closable value")
	elseif b == nil then
		_close_1 = nil
	else
		_close_1 = error("variable 'b' got a non-closable value")
	end
	(function(_arg_0, ...)
		local _ok_0 = _arg_0
		if _close_1 ~= nil then
			_close_1(b)
		end
		if _close_0 ~= nil then
			_close_0(a)
		end
		if _ok_0 then
			return ...
		else
			return error(...)
		end
	end)(pcall(function()
		local c, d = 123, 'abc'
		close(a, b)
		return const(c, d)
	end))
end
do
	local a = f()
	local b, c, d
	local _obj_0, _obj_1 = f1()
	b, c = _obj_0[1], _obj_0[2]
	d = _obj_1[1]
end
do
	local a, b, c, d
	local _obj_0, _obj_1, _obj_2 = f()
	a = _obj_0
	b, c = _obj_1[1], _obj_1[2]
	d = _obj_2[1]
end
do
	local a, b
	local _obj_0 = {
		2,
		3
	}
	a, b = _obj_0[1], _obj_0[2]
end
do
	local v
	if flag then
		v = func()
	else
		v = setmetatable({ }, {
			__close = function(self) end
		})
	end
	local _close_0
	if (function()
		local _val_0 = type(v)
		return 'table' == _val_0 or 'userdata' == _val_0
	end)() then
		_close_0 = assert(getmetatable(v) and getmetatable(v).__close, "variable 'v' got a non-closable value")
	elseif v == nil then
		_close_0 = nil
	else
		_close_0 = error("variable 'v' got a non-closable value")
	end
	(function(_arg_0, ...)
		local _ok_0 = _arg_0
		if _close_0 ~= nil then
			_close_0(v)
		end
		if _ok_0 then
			return ...
		else
			return error(...)
		end
	end)(pcall(function()
		local f
		do
			local _with_0 = io.open("file.txt")
			_with_0:write("Hello")
			f = _with_0
		end
		local _close_1
		if (function()
			local _val_0 = type(f)
			return 'table' == _val_0 or 'userdata' == _val_0
		end)() then
			_close_1 = assert(getmetatable(f) and getmetatable(f).__close, "variable 'f' got a non-closable value")
		elseif f == nil then
			_close_1 = nil
		else
			_close_1 = error("variable 'f' got a non-closable value")
		end
		return (function(_arg_0, ...)
			local _ok_0 = _arg_0
			if _close_1 ~= nil then
				_close_1(f)
			end
			if _ok_0 then
				return ...
			else
				return error(...)
			end
		end)(pcall(function() end))
	end))
end
do
	local a
	if true then
		a = 1
	end
	local b
	if not false then
		b = ((function()
			if x then
				return 1
			end
		end)())
	end
	local _close_0
	if (function()
		local _val_0 = type(b)
		return 'table' == _val_0 or 'userdata' == _val_0
	end)() then
		_close_0 = assert(getmetatable(b) and getmetatable(b).__close, "variable 'b' got a non-closable value")
	elseif b == nil then
		_close_0 = nil
	else
		_close_0 = error("variable 'b' got a non-closable value")
	end
	(function(_arg_0, ...)
		local _ok_0 = _arg_0
		if _close_0 ~= nil then
			_close_0(b)
		end
		if _ok_0 then
			return ...
		else
			return error(...)
		end
	end)(pcall(function()
		local c
		if true then
			c = ((function()
				local _exp_0 = x
				if "abc" == _exp_0 then
					return 998
				end
			end)())
		end
		local d
		if (function()
			if a ~= nil then
				return a
			else
				return b
			end
		end)() then
			d = {
				value = value
			}
		end
		local _close_1
		if (function()
			local _val_0 = type(d)
			return 'table' == _val_0 or 'userdata' == _val_0
		end)() then
			_close_1 = assert(getmetatable(d) and getmetatable(d).__close, "variable 'd' got a non-closable value")
		elseif d == nil then
			_close_1 = nil
		else
			_close_1 = error("variable 'd' got a non-closable value")
		end
		return (function(_arg_0, ...)
			local _ok_0 = _arg_0
			if _close_1 ~= nil then
				_close_1(d)
			end
			if _ok_0 then
				return ...
			else
				return error(...)
			end
		end)(pcall(function() end))
	end))
end
do
	local _
	do
		local _with_0 = io.open("file.txt")
		_with_0:write("Hello")
		_ = _with_0
	end
	local _close_0
	if (function()
		local _val_0 = type(_)
		return 'table' == _val_0 or 'userdata' == _val_0
	end)() then
		_close_0 = assert(getmetatable(_) and getmetatable(_).__close, "variable '_' got a non-closable value")
	elseif _ == nil then
		_close_0 = nil
	else
		_close_0 = error("variable '_' got a non-closable value")
	end
	(function(_arg_0, ...)
		local _ok_0 = _arg_0
		if _close_0 ~= nil then
			_close_0(_)
		end
		if _ok_0 then
			return ...
		else
			return error(...)
		end
	end)(pcall(function()
		local _ = setmetatable({ }, {
			__close = function()
				return print("second")
			end
		})
		local _close_1
		if (function()
			local _val_0 = type(_)
			return 'table' == _val_0 or 'userdata' == _val_0
		end)() then
			_close_1 = assert(getmetatable(_) and getmetatable(_).__close, "variable '_' got a non-closable value")
		elseif _ == nil then
			_close_1 = nil
		else
			_close_1 = error("variable '_' got a non-closable value")
		end
		return (function(_arg_0, ...)
			local _ok_0 = _arg_0
			if _close_1 ~= nil then
				_close_1(_)
			end
			if _ok_0 then
				return ...
			else
				return error(...)
			end
		end)(pcall(function()
			local _ = setmetatable({ }, {
				__close = function()
					return print("first")
				end
			})
			local _close_2
			if (function()
				local _val_0 = type(_)
				return 'table' == _val_0 or 'userdata' == _val_0
			end)() then
				_close_2 = assert(getmetatable(_) and getmetatable(_).__close, "variable '_' got a non-closable value")
			elseif _ == nil then
				_close_2 = nil
			else
				_close_2 = error("variable '_' got a non-closable value")
			end
			return (function(_arg_0, ...)
				local _ok_0 = _arg_0
				if _close_2 ~= nil then
					_close_2(_)
				end
				if _ok_0 then
					return ...
				else
					return error(...)
				end
			end)(pcall(function()
				return print("third")
			end))
		end))
	end))
end
local _defers = setmetatable({ }, {
	__close = function(self)
		self[#self]()
		self[#self] = nil
	end
})
local def
def = function(item)
	_defers[#_defers + 1] = item
	return _defers
end
do
	local _ = def(function()
		return print(3)
	end)
	local _close_0
	if (function()
		local _val_0 = type(_)
		return 'table' == _val_0 or 'userdata' == _val_0
	end)() then
		_close_0 = assert(getmetatable(_) and getmetatable(_).__close, "variable '_' got a non-closable value")
	elseif _ == nil then
		_close_0 = nil
	else
		_close_0 = error("variable '_' got a non-closable value")
	end
	return (function(_arg_0, ...)
		local _ok_0 = _arg_0
		if _close_0 ~= nil then
			_close_0(_)
		end
		if _ok_0 then
			return ...
		else
			return error(...)
		end
	end)(pcall(function()
		local _ = def(function()
			return print(2)
		end)
		local _close_1
		if (function()
			local _val_0 = type(_)
			return 'table' == _val_0 or 'userdata' == _val_0
		end)() then
			_close_1 = assert(getmetatable(_) and getmetatable(_).__close, "variable '_' got a non-closable value")
		elseif _ == nil then
			_close_1 = nil
		else
			_close_1 = error("variable '_' got a non-closable value")
		end
		return (function(_arg_0, ...)
			local _ok_0 = _arg_0
			if _close_1 ~= nil then
				_close_1(_)
			end
			if _ok_0 then
				return ...
			else
				return error(...)
			end
		end)(pcall(function()
			local _ = def(function()
				return print(1)
			end)
			local _close_2
			if (function()
				local _val_0 = type(_)
				return 'table' == _val_0 or 'userdata' == _val_0
			end)() then
				_close_2 = assert(getmetatable(_) and getmetatable(_).__close, "variable '_' got a non-closable value")
			elseif _ == nil then
				_close_2 = nil
			else
				_close_2 = error("variable '_' got a non-closable value")
			end
			return (function(_arg_0, ...)
				local _ok_0 = _arg_0
				if _close_2 ~= nil then
					_close_2(_)
				end
				if _ok_0 then
					return ...
				else
					return error(...)
				end
			end)(pcall(function() end))
		end))
	end))
end
