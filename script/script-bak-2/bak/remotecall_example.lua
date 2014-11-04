

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
  
  ctx,err = saker.popen(cmd)
  if err ~= nil then
    return False, err
  end
  return True,ctx
end

saker.register("system", "system", PROP_PASSIVITY)