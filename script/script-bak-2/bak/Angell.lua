

local Angelltable={}

function proc_status(Args)
  local sa = {}
  if Args ~= nil and #Args ~= 0 then
    for k,v in pairs(Args) 
    do
      sa[v] = Angelltable[v]
    end
  else 
    sa = Angelltable
  end
  
  local rsp=''
  for k,v in pairs(sa) 
  do
    pid,err = saker.pidof(v['pidfile'])
    if not pid then
        rsp=rsp..k.."\tStopping".."\tnil".."\r\n"
    else
        rsp=rsp..k.."\tRunning\t"..pid.."\r\n"
    end
  end
	return True,rsp
end


function proc_check(Args)
  local sa = {}
  if Args ~= nil and #Args ~= 0 then
    for k,v in pairs(Args) 
    do
        sa[v] = Angelltable[v]
    end
  else 
    sa = Angelltable
  end
  
  local rsp=''
  for k,v in pairs(sa) 
  do  
    if v["check"] == "yes" then
      pid,err = saker.pidof(v['pidfile'])
      if not pid then
        pid,err = saker.exec(v['pidfile'], v['scmd'], v['sparam'])
        if pid == nil then 
          rsp = "pull "..v['scmd'].."failed" 
          return False,err
        end
      end
    end
  end
	return True,rsp
end


function proc_start(Args)
  local sa = {}
  if Args ~= nil and #Args ~= 0 then
    for k,v in pairs(Args) 
    do
        sa[v] = Angelltable[v]
    end
  else 
    sa = Angelltable
  end
  local rsp = ""
  for k,v in pairs(sa)
	do
        v['check']="yes"
        pid,err = saker.pidof(v['pidfile'])
        if not pid then
           pid,err = saker.exec(v['pidfile'], v['scmd'], v['sparam'])
           if not pid then return False,err end              
           rsp = rsp.."start "..k.." success"
        end         
  end
  return True, rsp
end


function proc_stop(Args)
  local sa = {}
  if Args ~= nil and #Args ~= 0 then
    for k,v in pairs(Args) 
    do
        sa[v] = Angelltable[v]
    end
  else 
    sa = Angelltable
  end
  local rsp = ""
  for k,v in pairs(sa)
	do
    v['check']="no"
    pid,err = saker.pidof(v['pidfile'])
    if pid then
      if v['ksignal'] == 0 then
        os.execute(v['kcmd'].." "..v['kparam'])
      else 
        saker.kill(pid, v['ksignal'])
      end  
      rsp = rsp.."stop "..k.." success"
    end      
  end
  return True,rsp
end


if saker.uname() == "UNIX" then
  modulefile = saker.pwd().."/../script/ModuleAppsForLinux.json"
else 
  modulefile = saker.pwd().."/../script/ModuleAppsForWin.json"
end
local reader = assert(io.open(modulefile, "rb"))
local ctx = reader:read('*a')
io.close(reader)
Angelltable = json.decode(ctx)
saker.register("svstatus", "proc_status", PROP_PASSIVITY)
saker.register("svcheck", "proc_check", PROP_CYCLE)
saker.register("svstart", "proc_start", PROP_PASSIVITY)
saker.register("svstop", "proc_stop", PROP_PASSIVITY)


