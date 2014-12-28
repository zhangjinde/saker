
function procinfo(args)
    if type(args) ~= "table" then
        return false, "Wrong args"
    end
    
    for k,v in pairs(args)
    do
        local ctx,err = saker.sysinfo("system.proc.all", v)
        if err ~= nil then
            return false,err
        end
        return true,ctx
    end
end
    
saker.register("procinfo", "procinfo", PROP_PASSIVITY)
