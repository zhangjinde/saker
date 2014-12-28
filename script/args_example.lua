
function Args_Example(args)
    if type(args) ~= "table" then
      return false, "Wrong args"
    end
    for k,v in pairs(args) 
    do
    print(k..":"..v)
    end
    return true,"a","b"
end

saker.register("Args_Example", "Args_Example", PROP_PASSIVITY)