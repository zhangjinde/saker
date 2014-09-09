

local Angelltable={}
local modulefile = saker.pwd().."/../example/angell/angell.conf"
local tsvlife = {}

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
                local pwd = saker.pwd()
                if v['home'] ~= nil and v['home'] ~= "" then 
                    saker.cd(v['home'])
                end
                if v['adopt'] == "yes" then 
                    saker.adopt(v['pidfile'], v['scmd']) 
                else
                    saker.exec(v['scmd'])
                end  
                saker.cd(pwd)            
                pid,err = saker.pidof(v['pidfile'],v['key'])
            end  
            
            if pid == nil then 
                life_add(k, "start failed"..err)
                return false,err
            elseif pid ~= v['pid'] and v['pid'] ~= 0 then
                life_add(k, "may be crashed. last pid ["..v['pid'].."] ,current pid ["..pid.."]")                
            end
            v['pid'] = pid           
        elseif v["check"] == "wait" then 
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
            local pwd = saker.pwd()
            if v['home'] ~= nil and v['home'] ~= "" then 
                saker.cd(v['home'])
            end
            if v['adopt'] == "yes" then 
                saker.adopt(v['pidfile'], v['scmd']) 
            else
                saker.exec(v['scmd'])
            end  
            saker.cd(pwd)            
            pid,err = saker.pidof(v['pidfile'],v['key'])            
        end

        if pid == nil then 
            life_add(k, "user request start failed. "..err)
            return false,err
        end
        v['pid'] = pid
        life_add(k, "user request start. current pid ["..pid.."]")
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
            v['pid'] = 0
            life_add(k, "user request stop. current pid ["..pid.."]")
            rsp = rsp.."stop "..k.." success"
        end      
    end
    
    return true,rsp
end

function proc_life(Args) 
    for k,v in pairs(Args)
    do
        return true, json.encode(tsvlife[v])
    end
    return false, "cannot nil param"
end

function reload_config()
    local reader = assert(io.open(modulefile, "rb"))
    local ctx = reader:read('*a')
    io.close(reader)
    Angelltable = json.decode(ctx)
end

function life_add(k, a)
    if tsvlife[k] == nil then
        tsvlife[k] = {}
    end
    local idx = table.getn(tsvlife[k])
    tsvlife[k][idx+1] = {}
    tsvlife[k][idx+1]["index"] = idx + 1
    tsvlife[k][idx+1]["time"] = os.date("%a, %d %b %Y %X GMT")
    tsvlife[k][idx+1]["action"] = a 
end

function init_angelltable()
    for k, v in pairs(Angelltable)
    do
        v['pid'] = 0
        life_add(k, "saker supervised")
    end
end

reload_config()

init_angelltable()


saker.register("svstatus", "proc_status", PROP_PASSIVITY)
saker.register("svcheck",  "proc_check", PROP_CYCLE)
saker.register("svstart", "proc_start", PROP_PASSIVITY)
saker.register("svstop", "proc_stop", PROP_PASSIVITY)
saker.register("svlife", "proc_life", PROP_PASSIVITY)


