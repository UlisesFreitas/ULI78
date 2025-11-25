local _anon_func_0 = function(tb)
	return tb.func
end
local _anon_func_1 = function(tb)
	return tb.func()
end
local _anon_func_2 = function(tb)
	return tb.func()
end
local _anon_func_3 = function(tb)
	return (tb.func())
end
local _anon_func_4 = function(tb)
	return (tb:func(1, 2, 3))
end
local _anon_func_5 = function(tb)
	return tb.func(1)
end
local _anon_func_6 = function(tb)
	return tb.func(1)
end
local _anon_func_7 = function(a, b, c, tb)
	return tb.f(a, b, c)
end
local _anon_func_8 = function(_arg_0, ...)
	local ok = _arg_0
	return ...
end
local _anon_func_10 = function(_arg_0, ...)
	local _ok_0 = _arg_0
	if _ok_0 then
		return ...
	end
end
local _anon_func_9 = function(func, pcall)
	return _anon_func_10(pcall(func))
end
local _anon_func_12 = function(_arg_0, ...)
	local _ok_0 = _arg_0
	if _ok_0 then
		return ...
	end
end
local _anon_func_11 = function(func, pcall)
	return _anon_func_12(pcall(func))
end
local _anon_func_14 = function(_arg_0, ...)
	local _ok_0 = _arg_0
	if _ok_0 then
		return ...
	end
end
local _anon_func_13 = function(func, print, xpcall)
	return _anon_func_14(xpcall(function()
		print(123)
		return func()
	end, function(e)
		print(e)
		return e
	end))
end
local f
f = function()
	xpcall(function()
		return func(1, 2, 3)
	end, function(err)
		return print(err)
	end)
	xpcall(function()
		return func(1, 2, 3)
	end, function(err)
		return print(err)
	end)
	pcall(function()
		print("trying")
		return func(1, 2, 3)
	end)
	do
		local success, result = xpcall(function()
			return func(1, 2, 3)
		end, function(err)
			return print(err)
		end)
		success, result = pcall(function()
			return func(1, 2, 3)
		end)
	end
	local tb = { }
	pcall(_anon_func_0, tb)
	pcall(_anon_func_1, tb)
	pcall(_anon_func_2, tb)
	pcall(_anon_func_3, tb)
	pcall(_anon_func_4, tb)
	pcall(_anon_func_5, tb)
	pcall(_anon_func_6, tb)
	if (xpcall(function()
		return func(1)
	end, function(err)
		return print(err)
	end)) then
		print("OK")
	end
	if xpcall(function()
		return (func(1))
	end, function(err)
		return print(err)
	end) then
		print("OK")
	end
	do
		do
			local success, result = pcall(function()
				return func("abc", 123)
			end)
			if success then
				print(result)
			end
		end
		local success, result = xpcall(function()
			return func("abc", 123)
		end, function(err)
			return print(err)
		end)
		success, result = xpcall(function()
			return func("abc", 123)
		end, function(err)
			return print(err)
		end)
		if success then
			print(result)
		end
	end
	do
		pcall(function()
			return func(1, 2, 3)
		end)
		pcall(function()
			return func(1, 2, 3)
		end)
	end
	do
		x(function()
			local tb, a, b, c
			local f1
			f1 = function()
				return pcall(_anon_func_7, a, b, c, tb)
			end
		end)
	end
	do
		local f1
		f1 = function()
			do
				return _anon_func_8(pcall(function()
					return func()
				end))
			end
		end
	end
	do
		local func
		local a, b, c
		local _ok_0, _ret_0, _ret_1, _ret_2 = pcall(func)
		if _ok_0 then
			a, b, c = _ret_0, _ret_1, _ret_2
		end
	end
	do
		local a, b, c
		local _ok_0, _ret_0, _ret_1, _ret_2 = pcall(function()
			return func()
		end)
		if _ok_0 then
			a, b, c = _ret_0, _ret_1, _ret_2
		end
	end
	do
		local a
		local _exp_0 = (_anon_func_9(func, pcall))
		if _exp_0 ~= nil then
			a = _exp_0
		else
			a = "default"
		end
	end
	do
		f(_anon_func_11(func, pcall))
	end
	do
		f(_anon_func_13(func, print, xpcall))
	end
	return nil
end
local _anon_func_15 = function(a, b, c, tb)
	return tb.f(a, b, c)
end
local _anon_func_16 = function(_arg_0, ...)
	local ok = _arg_0
	return ...
end
do
	xpcall(function()
		return func(1, 2, 3)
	end, function(err)
		return print(err)
	end)
	xpcall(function()
		return func(1, 2, 3)
	end, function(err)
		return print(err)
	end)
	pcall(function()
		print("trying")
		return func(1, 2, 3)
	end)
	do
		local success, result = xpcall(function()
			return func(1, 2, 3)
		end, function(err)
			return print(err)
		end)
		success, result = pcall(function()
			return func(1, 2, 3)
		end)
	end
	local tb = { }
	pcall(function()
		return tb.func
	end)
	pcall(function()
		return tb.func()
	end)
	pcall(function()
		return tb.func()
	end)
	pcall(function()
		return (tb.func())
	end)
	pcall(function()
		return (tb:func(1, 2, 3))
	end)
	pcall(function()
		return tb.func(1)
	end)
	pcall(function()
		return tb.func(1)
	end)
	if (xpcall(function()
		return func(1)
	end, function(err)
		return print(err)
	end)) then
		print("OK")
	end
	if xpcall(function()
		return (func(1))
	end, function(err)
		return print(err)
	end) then
		print("OK")
	end
	do
		do
			local success, result = pcall(function()
				return func("abc", 123)
			end)
			if success then
				print(result)
			end
		end
		local success, result = xpcall(function()
			return func("abc", 123)
		end, function(err)
			return print(err)
		end)
		success, result = xpcall(function()
			return func("abc", 123)
		end, function(err)
			return print(err)
		end)
		if success then
			print(result)
		end
	end
	do
		pcall(function()
			return func(1, 2, 3)
		end)
		pcall(function()
			return func(1, 2, 3)
		end)
	end
	do
		x(function()
			local tb, a, b, c
			local f1
			f1 = function()
				return pcall(_anon_func_15, a, b, c, tb)
			end
		end)
	end
	do
		local f1
		f1 = function()
			do
				return _anon_func_16(pcall(function()
					return func()
				end))
			end
		end
	end
	do
		local func
		local a, b, c
		local _ok_0, _ret_0, _ret_1, _ret_2 = pcall(func)
		if _ok_0 then
			a, b, c = _ret_0, _ret_1, _ret_2
		end
	end
	do
		local a, b, c
		local _ok_0, _ret_0, _ret_1, _ret_2 = pcall(function()
			return func()
		end)
		if _ok_0 then
			a, b, c = _ret_0, _ret_1, _ret_2
		end
	end
	do
		local a
		local _exp_0 = ((function()
			return (function(_arg_0, ...)
				local _ok_0 = _arg_0
				if _ok_0 then
					return ...
				end
			end)(pcall(function()
				return func()
			end))
		end)())
		if _exp_0 ~= nil then
			a = _exp_0
		else
			a = "default"
		end
	end
	do
		f((function()
			return (function(_arg_0, ...)
				local _ok_0 = _arg_0
				if _ok_0 then
					return ...
				end
			end)(pcall(function()
				return func()
			end))
		end)())
	end
	do
		f((function()
			return (function(_arg_0, ...)
				local _ok_0 = _arg_0
				if _ok_0 then
					return ...
				end
			end)(xpcall(function()
				print(123)
				return func()
			end, function(e)
				print(e)
				return e
			end))
		end)())
	end
end
return nil
