
local FileName = "../bin/saker.log"

local current = 0

--[2013-09-23 21:05:50] [TRACE] [../../src/saker.c:39] Do task [SysInfo_Proc]
--if use "[INFO]" , that use match pattern
local matchlist={"INFO", "ERROR"}

function tailf()

  saker.log(LOG_TRACE, "this is tailf")
  local reader = assert(io.open(FileName, "rb"))
  
	local size    = reader:seek("end")
	if current == 0 and size > 500 then
        current = size - 500
  elseif current > size then
        if size > 500 then 
        current = size - 500
        else 
        current = 0
        end
  end
  
	reader:seek("set", current)
	if current == size then
		return
	end
  
  --- also you can   reader:read("*all") ,that will look like tailf more
  local isMatch = true
  local logmsg = ""
  while reader:seek("cur") < size 
  do
	   ctx = reader:read("*line")
     isMatch = false
     -- also you can filter something
     for k,v in pairs(matchlist) 
     do       
         if string.find(ctx, v) ~= nil then
             isMatch = true
         end
     end
     
	   if isMatch then 
         ctx = string.gsub(ctx, "\r", "")
         ctx = string.gsub(ctx, "\n", "")   
         logmsg=logmsg.."\r\n"..ctx
     end
  end
  
  if #logmsg ~= 0 then
      saker.publish("log", logmsg) 
  end
  current = reader:seek("cur")
  io.close(reader)
  
  return True
end

saker.register("tailf", "tailf", PROP_CYCLE)
