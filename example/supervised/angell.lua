local Moduledir = "/etc/saker"
local Angelltable = {}
local OrderAngelltable = {}
local Modulelife = {}
local Modulegroup = {}

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
        obj["code"] = 0
        
        if not pid then            
            obj["STATUS"]= "Stopped"
            obj["PID"] = "nil"
            obj["MEM"] = 0
            obj["CPU"]= 0
            obj["UPTIME"] = 0
            if v['check'] == 'yes' then
                obj["code"] = 1
                obj["STATUS"] = "Abnormal"
            end
        else
            local infostr,err = saker.sysinfo("system.proc.all", pid)
            if infostr == nil then 
                return false,err 
            end
            local infotb = json.decode(infostr)
            
            if v['check'] == 'yes' then
                obj["STATUS"] = "Running"
            else 
                obj["code"] = 1
                obj["STATUS"] = "Revolting"
            end
            
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
    local ordersa = {}
    if Args ~= nil and #Args ~= 0 then
        for _,v in pairs(Args)
        do
            if Angelltable[v] ~= nil then
                table.insert(ordersa, Angelltable[v])
            end
        end
        table_sort(ordersa)
    else
        ordersa = OrderAngelltable
    end

    for _,v in ipairs(ordersa)
    do
        local pid
        local err
        if v["check"] == "yes" then
            pid,err = saker.pidof(v['pidfile'],v['key'])
            if not pid then
                local pwd = saker.pwd()
                if v['home'] ~= nil and v['home'] ~= "" then
                    saker.cd(v['home'])
                end
                
                local tstart = os.time()   
                
                if v['adopt'] == "yes" then
                    if v['user'] ~= nil and v['user'] ~= "" then
                        saker.adopt(v['pidfile'], v['scmd'], v['user'])
                    else
                        saker.adopt(v['pidfile'], v['scmd'])
                    end
                else
                    saker.log(LOG_DEBUG, v['scmd'].." start")
                    saker.exec(v['scmd'], v['waitstart'])
                    saker.log(LOG_DEBUG, v['scmd'].." stop")
                end
                saker.cd(pwd)
                saker.log(LOG_DEBUG, v['scmd'].." waitstart enter")
                --- 当达到超时时间，或者负数时(系统时间被修改) 则break
                while os.difftime(os.time(), tstart) >= 0 and os.difftime(os.time(), tstart) < v['waitstart']/1000 do
                    pid,_ = saker.pidof(v['pidfile'], v['key'])
                    if pid then
                        break
                    end
                    saker.sleep(100)
                end
                saker.log(LOG_DEBUG, v['scmd'].." waitstart laeve")
                
                pid,err = saker.pidof(v['pidfile'], v['key'])
                if pid then 
                    saker.log(LOG_INFO, v['name'].." start.")
                end
            end
            
            if pid ~= v['pid'] and v['pid'] ~= 0 then
                saker.log(LOG_ERROR, v['name'].." may be crashed. last pid ["..tostring(v['pid']).."]")
                svinfo_add(v['name'], " may be crashed. last pid ["..tostring(v['pid']).."] ,current pid ["..tostring(pid).."]")             
            end
            
            if pid ~= v['pid'] and pid == nil then
                local errmsg = "start failed. "
                if err then
                    errmsg = errmsg..err
                end
                saker.log(LOG_ERROR, v['name'].." "..errmsg)
            end
            
            v['pid'] = pid
        elseif v["check"] == "wait" then
            pid,err = saker.pidof(v['pidfile'], v['key'])
            if pid then
                svinfo_add(v['name'], " prev stop failed. so force stop")
                saker.kill(pid, 9)
                os.remove(v['pidfile'])
            end
            v["check"] = "no"
        end
    end

    return true,""
end

function proc_start(Args)
    local rsp = {}
    
    if Args == nil or #Args == 0 then
        rsp['nil'] = {}
        rsp['nil']['code'] = 1
        rsp['nil']['desc'] = 'wrong param'
        return true, json.encode(rsp)
    end
    
    local sa = {}
    for _,name in pairs(Args)
    do
        if Angelltable[name] == nil then
            rsp[name] = {}
            rsp[name]['code'] = 1
            rsp[name]['desc'] = 'cannot found'
        else 
            sa[name] = Angelltable[name]
        end
    end

    for name,v in pairs(sa)
    do
        rsp[name] = {}
        local pid
        if v["check"] == "wait" then
            pid,err = saker.pidof(v['pidfile'], v['key'])
            if pid then
                svinfo_add(name, " prev stop failed. so force stop")
                saker.kill(pid, 9)
                os.remove(v['pidfile'])
            end
        end
        v['check']="yes"
        pid,err = saker.pidof(v['pidfile'], v['key'])
        if not pid then
            local pwd = saker.pwd()
            if v['home'] ~= nil and v['home'] ~= "" then
                saker.cd(v['home'])
            end
            local tstart = os.time()
            if v['adopt'] == "yes" then
                if v['user'] ~= nil and v['user'] ~= "" then
                    saker.adopt(v['pidfile'], v['scmd'], v['user'])
                else
                    saker.adopt(v['pidfile'], v['scmd'])
                end
            else
                saker.exec(v['scmd'], v['waitstart'])
            end
            saker.cd(pwd)
            
            while os.difftime(os.time(), tstart) >= 0 and os.difftime(os.time(), tstart) < v['waitstart']/1000 do                
                pid,_ = saker.pidof(v['pidfile'], v['key'])
                if pid then
                    break
                end
                --- avoid high CPU load
                saker.sleep(100) 
            end
            
            pid,err = saker.pidof(v['pidfile'], v['key'])
            if pid == nil then
                rsp[name]['code'] = 1
                rsp[name]['desc'] = err
                local errmsg = "user request start failed. "
                if err then 
                    errmsg=errmsg..err
                end
                svinfo_add(name, errmsg)  
            else 
                rsp[name]['code'] = 0
                rsp[name]['desc'] = 'start ok, pid:'..pid
            end            
        else 
            rsp[name]['code'] = 0
            rsp[name]['desc'] = 'has started, pid:'..pid
        end
        
        if pid ~= nil then
            v['pid'] = pid
            svinfo_add(name, "user request start. current pid ["..tostring(pid).."]")
        end
    end
    
    return true, json.encode(rsp)
end

function proc_stop(Args)
    local rsp = {}
    
    if Args == nil or #Args == 0 then
        rsp['nil'] = {}
        rsp['nil']['code'] = 1
        rsp['nil']['desc'] = 'wrong param'
        return true, json.encode(rsp)
    end
 
    local sa = {}
    for _,name in pairs(Args)
    do
        if Angelltable[name] == nil then
            rsp[name] = {}
            rsp[name]['code'] = 1
            rsp[name]['desc'] = 'cannot found'
        else 
            sa[name] = Angelltable[name]
        end
    end
    
    for name,v in pairs(sa)
    do
        rsp[name] = {}
        
        v['check']="wait"
        local pid,err = saker.pidof(v['pidfile'], v['key'])
        if pid then
            local tstart = os.time()
            
            if v['ksignal'] == 0 then
                local pwd = saker.pwd()
                if v['home'] ~= nil and v['home'] ~= "" then
                    saker.cd(v['home'])
                end
                saker.exec(v['kcmd'], v['waitstop'])
                saker.cd(pwd)
            else
                saker.kill(pid, v['ksignal'])
            end
            v['pid'] = 0
            svinfo_add(name, "user request stop. current pid ["..tostring(pid).."]")
            
            while os.difftime(os.time(), tstart) >= 0 and os.difftime(os.time(), tstart) < v['waitstop']/1000 do
                pid,_ = saker.pidof(v['pidfile'], v['key'])
                if not pid then
                    break
                end
                --- avoid high CPU load
                saker.sleep(100) 
            end

            pid,err = saker.pidof(v['pidfile'], v['key'])
            if pid then 
                saker.log(LOG_ERROR, "stop ["..name.."] blocked , so waiting in the background")
                rsp[name]['code'] = 0
                rsp[name]['desc'] = 'blocked, so waiting in the background' 
            else 
                rsp[name]['code'] = 0
                rsp[name]['desc'] = 'ok'
            end           
        else 
            rsp[name]['code'] = 0
            rsp[name]['desc'] = "has stopped"
        end
    end
    
    return true,json.encode(rsp)
end

function proc_drop(Args)
    local rsp = {}
    
    if Args == nil or #Args == 0 then
        rsp['nil'] = {}
        rsp['nil']['code'] = 1
        rsp['nil']['desc'] = 'wrong param'
        return true, json.encode(rsp)
    end
    
    local sa = {}
    for _,name in pairs(Args)
    do
        if Angelltable[name] == nil then
            rsp[name] = {}
            rsp[name]['code'] = 1
            rsp[name]['desc'] = 'cannot found'
        else 
            sa[name] = Angelltable[name]
        end
    end
    
    for name,sitem in pairs(sa)
    do
        sitem['check']="no"
        rsp[name] = {}
        rsp[name]['code'] = 0
        rsp[name]['desc'] = 'ok'
    end
    
    return true, json.encode(rsp)
end

function proc_restart(Args)
    local rsp = {}
    
    if Args == nil or #Args == 0 then
        rsp['nil'] = {}
        rsp['nil']['code'] = 1
        rsp['nil']['desc'] = 'wrong param'
        return true, json.encode(rsp)
    end
    
    local sa = {}
    local rsp = {}
    for _,name in pairs(Args)
    do
        if Angelltable[name] == nil then
            rsp[name] = {}
            rsp[name]['code'] = 1
            rsp[name]['desc'] = 'cannot found'
        else 
            sa[name] = Angelltable[name]
        end
    end
    
    local ok, rst = proc_stop(Args)
    if not ok then
        return false, rst
    end
    
    local ok, rst = proc_start(Args) 
    if not ok then
        return false, rst
    end
    
    return true, rst
end

function proc_tryrestart(Args)
    local rsp = {}
    
    if Args == nil or #Args == 0 then
        rsp['nil'] = {}
        rsp['nil']['code'] = 1
        rsp['nil']['desc'] = 'wrong param'
        return true, json.encode(rsp)
    end
    
    local sa = {}
    for _,name in pairs(Args)
    do
        if Angelltable[name] == nil then
            rsp[name] = {}
            rsp[name]['code'] = 1
            rsp[name]['desc'] = 'cannot found'
        else 
            sa[name] = Angelltable[name]
        end
    end
    
    for name,v in pairs(sa)
    do
        if v['check'] == "yes" then
            local tmptbl = {}
            table.insert(tmptbl, name)
            local isok, rst = proc_restart(tmptbl)
            rsp[name] = {}
            if isok then
                rsp[name]['code'] = 0
                rsp[name]['desc'] = 'ok'
            else 
                rsp[name]['code'] = 1
                rsp[name]['desc'] = 'restart failed'
            end
        else 
            rsp[name] = {}
            rsp[name]['code'] = 0
            rsp[name]['desc'] = 'is detachment'
        end
    end
    
    return true, json.encode(rsp)
end

function proc_group_start(Args)
    local rsp = {}
    
    if Args == nil or #Args == 0 then
        rsp['nil'] = {}
        rsp['nil']['code'] = 1
        rsp['nil']['desc'] = 'wrong param'
        return true, json.encode(rsp)
    end
    
    for _, gname in pairs(Args) 
    do 
        local group = Modulegroup[gname]
        if group ~= nil then
            rsp[gname] = {}
            local isok, ret = proc_start(group)
            if not isok then
                rsp[gname]['code'] = 1
                rsp[gname]['desc'] = 'start failed'
            else 
                rsp[gname]['code'] = 0
                rsp[gname]['desc'] = 'ok'
            end
        else 
            rsp[gname] = {}
            rsp[gname]['code'] = 1
            rsp[gname]['desc'] = 'cannot found'
        end
    end
    
    return true, json.encode(rsp)
end

function proc_group_stop(Args)
    local rsp = {}
    
    if Args == nil or #Args == 0 then
        rsp['nil'] = {}
        rsp['nil']['code'] = 1
        rsp['nil']['desc'] = 'wrong param'
        return true, json.encode(rsp)
    end
    
    for _, gname in pairs(Args) 
    do 
        local group = Modulegroup[gname]
        if group ~= nil then
            local isok, err = proc_stop(group)
            rsp[gname] = {}
            if not isok then
                rsp[gname]['code'] = 1
                rsp[gname]['desc'] = 'start failed'
            else 
                rsp[gname]['code'] = 0
                rsp[gname]['desc'] = 'ok'
            end
        else 
            rsp[gname] = {}
            rsp[gname]['code'] = 1
            rsp[gname]['desc'] = 'cannot found'
        end
    end
    
    return true, json.encode(rsp)
end

function proc_group_restart(Args)
    local rsp = {}
    
    if Args == nil or #Args == 0 then
        rsp['nil'] = {}
        rsp['nil']['code'] = 1
        rsp['nil']['desc'] = 'wrong param'
        return true, json.encode(rsp)
    end
    
    for _, gname in pairs(Args) 
    do 
        local group = Modulegroup[gname]
        if group ~= nil then
            local isok, err = proc_restart(group)
            rsp[gname] = {}
            if not isok then
                rsp[gname]['code'] = 1
                rsp[gname]['desc'] = 'restart failed'
            else 
                rsp[gname]['code'] = 0
                rsp[gname]['desc'] = 'ok'
            end
        else 
            rsp[gname] = {}
            rsp[gname]['code'] = 1
            rsp[gname]['desc'] = 'cannot found'
        end
    end
    
    return true, json.encode(rsp)
end

function proc_group_member(Args)
    local rsp = {}
    
    if Args == nil or #Args == 0 then
        rsp['nil'] = {}
        rsp['nil']['code'] = 1
        rsp['nil']['desc'] = 'wrong param'
        return true, json.encode(rsp)
    end
    
    for _, gname in pairs(Args) 
    do 
        rsp = Modulegroup[gname]
    end
    
    return true, json.encode(rsp)
end

function proc_group_status(Args)
    local rsp = {}
    
    if Args == nil or #Args == 0 then
        rsp['nil'] = {}
        rsp['nil']['code'] = 1
        rsp['nil']['desc'] = 'wrong param'
        return true, json.encode(rsp)
    end
    
    for _, gname in pairs(Args) 
    do 
        --- group
        rsp[gname] = {}
        rsp[gname]['code'] = 0
        rsp[gname]['desc'] = 'ok'
        for _,name in pairs(Modulegroup[gname]) 
        do
            local item = Angelltable[name]
            local pid, err = saker.pidof(item['pidfile'], item['key'])
            --- revolting
            if item['check'] ~= 'yes' and pid then
                rsp[gname]['desc'] = name..' revolting.'
                rsp[gname]['code'] = 1
            end
            
            --- abnormal
            if item['check'] == 'yes' and not pid then
                rsp[gname]['desc'] = name..' abnormal.'
                rsp[gname]['code'] = 1                
            end
        end
    end
    
    return true, json.encode(rsp)
end

function proc_info(Args)
    for _,sname in pairs(Args)
    do
        return true, json.encode(Modulelife[sname])
    end
    local rsp = {}
    rsp['code'] = 0
    return false, json.encode(rsp)
end

function svinfo_add(sname, a)
    if Modulelife[sname] == nil then
        Modulelife[sname] = {}
        Modulelife[sname.."idx"] = 0
    end

    local maxidx = Modulelife[sname.."idx"]
    if maxidx > 1000 then
        table.remove(Modulelife[sname], 1)
    end
    maxidx = maxidx + 1
    if maxidx < 0 then
        maxidx = 0
    end
    local rs = {}
    rs["index"] = maxidx 
    rs["time"] = os.date("%a, %d %b %Y %X GMT")
    rs["action"] = a
    table.insert(Modulelife[sname], rs)
    Modulelife[sname.."idx"] = maxidx
end

function config_load()
    local files,err = saker.walk(Moduledir)    
    if files == nil then 
        return false, err
    end
    
    for _,filename in pairs(files) 
    do
        local suffix = string.sub(filename, #filename-2, -1)
        if suffix == "svr" then 
            saker.log(LOG_INFO, "parse "..filename)
            local reader = assert(io.open(filename, "rb"))
            local ctx = reader:read('*a')
            io.close(reader)
            local object = {}
            local status, err = pcall(function() object = json.decode(ctx) end)
            if not status then
                saker.log(LOG_ERROR, "invaild json. ")
                if err then
                    saker.log(err)
                end
            else 
                for sname, sitem in pairs(object)
                do
                    Angelltable[sname] = sitem                    
                end
            end
        end
    end
end

function group_insert(tbl, eml)
    for _, v in pairs(tbl)
    do
        --- 如果已经存在，则直接返回
        if v == eml then
            return false
        end
    end
    table.insert(tbl, eml)
    return true
end

function sort_func(lhs, rhs)
    return lhs['priority'] < rhs['priority']
end

function table_sort(tbl)
    table.sort(tbl, sort_func)
end

function angelltable_init()
    Modulegroup["default"] = {}
    OrderAngelltable = {}
    for sname, sitem in pairs(Angelltable) 
    do
        sitem['name'] = sname
        table.insert(OrderAngelltable, sitem)
    end
    
    for _, sitem in ipairs(OrderAngelltable) 
    do 
        --- 初始化进程相关信息
        sitem['pid'] = 0
        --- 默认最低优先级(0~99 0为最高)
        if sitem['priority'] == nil then
            sitem['priority'] = 99
        end
        --- 默认3s阻塞等待时间
        if sitem['waitstart'] == nil or sitem['waitstart'] < 0 then
            sitem['waitstart'] = 3000
        --- 最大30s阻塞等待
        elseif  sitem['waitstart'] > 30000 then
            sitem['waitstart'] = 30000
        end

        --- 默认3s阻塞等待时间
        if sitem['waitstop'] == nil or sitem['waitstop'] > 0 then
            sitem['waitstop'] = 3000
        --- 最大30s阻塞等待时间
        elseif sitem['waitstop'] > 30000 then
            sitem['waitstop'] = 30000
        end
    end
    
    table_sort(OrderAngelltable)
    
    for _,sitem in ipairs(OrderAngelltable)
    do
        saker.log(LOG_INFO, sitem['name'].." priority:"..sitem['priority'])
    end
    
    for _, sitem in ipairs(OrderAngelltable) 
    do 
        --- 所有均加入default组 
        group_insert(Modulegroup["default"], sitem['name'])
        
        if sitem['group'] ~= nil then
            for _, gname in pairs(sitem['group'])
            do
                if Modulegroup[gname] ~= nil then
                    group_insert(Modulegroup[gname], sitem['name'])
                else 
                    Modulegroup[gname] = {}
                    group_insert(Modulegroup[gname], sitem['name'])
                end
            end
        end
        
        svinfo_add(sitem['name'], " saker supervised.")
    end
end

function sv_reload()
    Angelltable = {}
    Modulegroup = {}
    config_load()
    angelltable_init()
    
    local rsp = {}  
    rsp['nil'] = {}
    rsp['nil']['code'] = 0
    rsp['nil']['desc'] = 'ok'
    return true, json.encode(rsp)
end

sv_reload()

saker.register("svreload",   "sv_reload",           PROP_PASSIVITY)
saker.register("svcheck",    "proc_check",          PROP_CYCLE)
saker.register("svstart",    "proc_start",          PROP_PASSIVITY)
saker.register("svstop",     "proc_stop",           PROP_PASSIVITY)
saker.register("svrestart",  "proc_restart",        PROP_PASSIVITY)
saker.register("svdrop",     "proc_drop",           PROP_PASSIVITY)
saker.register("svtryrestart",  "proc_tryrestart",  PROP_PASSIVITY)

saker.register("svgstart",   "proc_group_start",    PROP_PASSIVITY)
saker.register("svgstop",    "proc_group_stop",     PROP_PASSIVITY)
saker.register("svgrestart", "proc_group_restart",  PROP_PASSIVITY)

saker.register("svgmember",  "proc_group_member",   PROP_PASSIVITY)
saker.register("svgstatus",  "proc_group_status",   PROP_PASSIVITY)

saker.register("svstatus",   "proc_status",         PROP_PASSIVITY)
saker.register("svlife",     "proc_info",           PROP_PASSIVITY)
