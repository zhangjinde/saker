
function Args_Example(args)
  if type(args) ~= "table" then
      return False, "Wrong args"
  end
  for k,v in pairs(args) 
  do
    print(k..":"..v)
  end
  return True,"a","b"
end

saker.register("Args_Example", "Args_Example", PROP_PASSIVITY)