
function system(args)
    if type(args) ~= "table" then
        return False, "Wrong args"
    end
    local cmd = ""
    for k,v in pairs(args) 
    do
        cmd = cmd.." "..v
    end
    print(cmd)
    --ret = os.execute(cmd)

    local ctx,err = saker.popen(cmd)
    if err ~= nil then
        return false, err
    end
    return true,ctx
end

saker.register("system", "system", PROP_PASSIVITY)
