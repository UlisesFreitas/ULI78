local _module_0 = { }
local hi = "hello"
local hello = "what the heckyes"
print(hi)
local umm = 'umm'
local here, another = "yeah", 'world'
local aye = "YU'M"
you('"hmmm" I said')
print(aye, you)
another = [[ hello world ]]
local hi_there = [[  hi there
]]
local well = [==[ "helo" ]==]
local hola = [===[  eat noots]===]
local mm = [[well trhere]]
local txt = [[

nil
Fail to compile
]]
txt[ [[abc]]] = [["#{i}" for i = 1, 10] for i = 1, 10]]
local oo = ""
local x = "\\"
x = "a\\b"
x = "\\\n"
x = "\""
local a = "hello " .. tostring(hello) .. " hello"
local b = tostring(hello) .. " hello"
local c = "hello " .. tostring(5 + 1)
local d = tostring(hello(world))
local e = tostring(1) .. " " .. tostring(2) .. " " .. tostring(3)
local f = [[hello #{world} world]]
local g = "#{hello world}"
a = 'hello #{hello} hello'
b = '#{hello} hello'
c = 'hello #{hello}'
local _ = "hello";
("hello"):format(1);
("hello"):format(1, 2, 3);
("hello"):format(1, 2, 3)(1, 2, 3);
("hello"):world();
("hello"):format().hello(1, 2, 3);
("hello"):format(1, 2, 3)
something("hello"):world()
something(("hello"):world())
do
	local str = "key: value"
	str = "config:\n\tenabled: true\n\tlevel: 5"
	str = "header: start\nfooter: end"
	str = "name: " .. tostring(username)
	str = "count: " .. tostring(total) .. " items"
	str = "user: " .. tostring(name) .. "\nid: " .. tostring(id)
	str = "path: \"C:\\\\Program Files\\\\App\"\ndesc: 'single \"quote\" test'"
	str = "key: value   \nnext: 123   "
	str = "list:\n  - \"one\"\n  - \"two\""
	str = "-- comment\ncontent text\n-- comment"
	str = tostring(1 + 2) .. '\n' .. tostring(2 + 3) .. '\n' .. tostring("a" .. "b")
	local obj = {
		settings = "mode: " .. tostring(mode) .. "\nflags:\n\t- " .. tostring(flag1) .. "\n\t- default"
	}
	local fn
	fn = function()
		return "Hello\nname: " .. tostring(userName)
	end
	str = "result:\n\tstatus: " .. tostring((function()
		if ok then
			return "pass"
		else
			return "fail"
		end
	end)()) .. "\n\tcode: " .. tostring(code)
	local summary = "date: " .. tostring(os.date()) .. "\nvalues:\n\t-\n\t\ta: " .. tostring(aVal) .. "\n\t\tb: " .. tostring(bVal or defaultB)
	local msg = send("Hello, " .. tostring(user) .. "!\nToday is " .. tostring(os.date("%A")) .. ".")
	local desc
	do
		local prefix = "Result"
		desc = tostring(prefix) .. ":\nvalue: " .. tostring(compute())
	end
	print(("1\n2\n3"))
end
local yaml = "version: " .. tostring(ver) .. "\nok: true"
_module_0["yaml"] = yaml
return _module_0
