local Props
do
	local _class_0
	local assignReadOnly
	local _base_0 = {
		__index = function(self, name)
			local cls = getmetatable(self)
			do
				local item
				do
					local _obj_0 = cls.__getter
					if _obj_0 ~= nil then
						item = _obj_0[name]
					end
				end
				if item then
					return item(self)
				else
					item = rawget(cls, name)
					if item then
						return item
					else
						local c = cls
						repeat
							c = getmetatable(c)
							if c then
								local _obj_0 = c.__getter
								if _obj_0 ~= nil then
									item = _obj_0[name]
								end
								if item then
									if cls.__getter == nil then
										cls.__getter = { }
									end
									cls.__getter[name] = item
									return item(self)
								else
									item = rawget(c, name)
									if item then
										rawset(cls, name, item)
										return item
									end
								end
							else
								break
							end
						until false
					end
				end
			end
			return nil
		end,
		__newindex = function(self, name, value)
			local cls = getmetatable(self)
			local item
			local _obj_0 = cls.__setter
			if _obj_0 ~= nil then
				item = _obj_0[name]
			end
			if item then
				return item(self, value)
			else
				local c = cls
				repeat
					c = getmetatable(c)
					if c then
						local _obj_1 = c.__setter
						if _obj_1 ~= nil then
							item = _obj_1[name]
						end
						if item then
							if cls.__setter == nil then
								cls.__setter = { }
							end
							cls.__setter[name] = item
							item(self, value)
							return
						end
					else
						break
					end
				until false
				return rawset(self, name, value)
			end
		end,
		prop = function(self, name, props)
			local get, set = props.get, props.set
			if set == nil then
				set = assignReadOnly
			end
			do
				local getter = rawget(self.__base, "__getter")
				if getter then
					getter[name] = get
				else
					rawset(self.__base, "__getter", {
						[name] = get
					})
				end
			end
			local setter = rawget(self.__base, "__setter")
			if setter then
				setter[name] = set
			else
				return rawset(self.__base, "__setter", {
					[name] = set
				})
			end
		end
	}
	if _base_0.__index == nil then
		_base_0.__index = _base_0
	end
	_class_0 = setmetatable({
		__init = function() end,
		__base = _base_0,
		__name = "Props"
	}, {
		__index = _base_0,
		__call = function(cls, ...)
			local _self_0 = setmetatable({ }, _base_0)
			cls.__init(_self_0, ...)
			return _self_0
		end
	})
	_base_0.__class = _class_0
	local self = _class_0;
	assignReadOnly = function()
		return error("assigning a readonly property")
	end
	Props = _class_0
end
local A
do
	local _class_0
	local _parent_0 = Props
	local _base_0 = { }
	for _key_0, _val_0 in pairs(_parent_0.__base) do
		if _base_0[_key_0] == nil and _key_0:match("^__") and not (_key_0 == "__index" and _val_0 == _parent_0.__base) then
			_base_0[_key_0] = _val_0
		end
	end
	if _base_0.__index == nil then
		_base_0.__index = _base_0
	end
	setmetatable(_base_0, _parent_0.__base)
	_class_0 = setmetatable({
		__init = function(self)
			self._x = 0
		end,
		__base = _base_0,
		__name = "A",
		__parent = _parent_0
	}, {
		__index = function(cls, name)
			local val = rawget(_base_0, name)
			if val == nil then
				local parent = rawget(cls, "__parent")
				if parent then
					return parent[name]
				end
			else
				return val
			end
		end,
		__call = function(cls, ...)
			local _self_0 = setmetatable({ }, _base_0)
			cls.__init(_self_0, ...)
			return _self_0
		end
	})
	_base_0.__class = _class_0
	local self = _class_0;
	self:prop('x', {
		get = function(self)
			return self._x + 1000
		end,
		set = function(self, v)
			self._x = v
		end
	})
	if _parent_0.__inherited then
		_parent_0.__inherited(_parent_0, _class_0)
	end
	A = _class_0
end
local B
do
	local _class_0
	local _parent_0 = A
	local _base_0 = { }
	for _key_0, _val_0 in pairs(_parent_0.__base) do
		if _base_0[_key_0] == nil and _key_0:match("^__") and not (_key_0 == "__index" and _val_0 == _parent_0.__base) then
			_base_0[_key_0] = _val_0
		end
	end
	if _base_0.__index == nil then
		_base_0.__index = _base_0
	end
	setmetatable(_base_0, _parent_0.__base)
	_class_0 = setmetatable({
		__init = function(self, ...)
			return _class_0.__parent.__init(self, ...)
		end,
		__base = _base_0,
		__name = "B",
		__parent = _parent_0
	}, {
		__index = function(cls, name)
			local val = rawget(_base_0, name)
			if val == nil then
				local parent = rawget(cls, "__parent")
				if parent then
					return parent[name]
				end
			else
				return val
			end
		end,
		__call = function(cls, ...)
			local _self_0 = setmetatable({ }, _base_0)
			cls.__init(_self_0, ...)
			return _self_0
		end
	})
	_base_0.__class = _class_0
	local self = _class_0;
	self:prop('abc', {
		get = function(self)
			return "hello"
		end
	})
	if _parent_0.__inherited then
		_parent_0.__inherited(_parent_0, _class_0)
	end
	B = _class_0
end
local b = B()
b.x = 999
return print(b.x, b.abc)
