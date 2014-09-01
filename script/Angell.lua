

local Angelltable={}
local modulefile

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
  
  local rsp={}
  for k,v in pairs(sa) 
  do
    local pid,err = saker.pidof(v['pidfile'], v['key'])
    local obj = {}
    if not pid then    
        obj["STATUS"]= "Stopped"
        obj["PID"] = "nil"
        obj["MEM"] = 0
        obj["CPU"]= 0
        obj["UPTIME"] = 0
    else
        local infostr,err = saker.sysinfo("system.proc.all", pid)
        if infostr == nil then print(err) return false,err end
        local infotb = json.decode(infostr)
        obj["STATUS"]= "Running"
        obj["PID"]= pid
        obj["MEM"]= string.format("%.2f",infotb["PMEM"]);
        obj["CPU"]= string.format("%.2f",infotb["PCPU"]);
        obj["UPTIME"] = "unknown"
        local fileinfo,err = saker.fileinfo(v['pidfile'], true)
        if fileinfo ~= nil then 
            obj["UPTIME"] = os.difftime(os.time(), fileinfo["MTIME"]).."s"
        end      
    end
    rsp[k] = obj
  end
  return true,json.encode(rsp)
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
  local pid
  local err
  for k,v in pairs(sa) 
  do  
    if v["check"] == "yes" then
      pid,err = saker.pidof(v['pidfile'],v['key'])
      if not pid then
           if v['daemon'] == "no" then 
               saker.adopt(v['pidfile'], v['scmd']) 
           else
               saker.exec(v['scmd'])
           end               
           pid,err = saker.pidof(v['pidfile'],v['key'])
      end
      if pid == nil then 
          saker.log(LOG_ERROR, err)
          return false,err
      end      
    else if v["check"] == "wait" then 
       pid,err = saker.pidof(v['pidfile'], v['key'])
       if pid then
            saker.kill(pid, 9)
       end
       v["check"] = "no"
    end
  end
  return true
end


function proc_start(Args)
  reload_config()
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
        local pid,err = saker.pidof(v['pidfile'], v['key'])
        if not pid then
           if v['daemon'] == "no" then 
              saker.adopt(v['pidfile'], v['scmd']) 
           else
              saker.exec(v['scmd'])
           end   
           pid,err = saker.pidof(v['pidfile'],v['key'])
        end
        if pid == nil then 
          saker.log(LOG_ERROR, err)
          return false,err
        end     
  end
  return true, rsp
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
    v['check']="wait"
    local pid,err = saker.pidof(v['pidfile'], v['key'])
    if pid then
      if v['ksignal'] == 0 then
        os.execute(v['kcmd'])
      else 
        saker.kill(pid, v['ksignal'])
      end  
      rsp = rsp.."stop "..k.." success"
    end      
  end
  return true,rsp
end

function reload_config()
  local reader = assert(io.open(modulefile, "rb"))
  local ctx = reader:read('*a')
  io.close(reader)
  Angelltable = json.decode(ctx)
end


if saker.uname() == "UNIX" then
  modulefile = saker.pwd().."/../script/ModuleAppsForLinux.json"
else 
  modulefile = saker.pwd().."/../script/ModuleAppsForWin.json"
end

reload_config()

saker.register("svstatus", "proc_status", PROP_PASSIVITY)
saker.register("svcheck",  "proc_check", PROP_CYCLE)
saker.register("svstart", "proc_start", PROP_PASSIVITY)
saker.register("svstop", "proc_stop", PROP_PASSIVITY)


