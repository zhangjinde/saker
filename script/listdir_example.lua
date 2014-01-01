
function ListFile_Example()
  saker.log(LOG_INFO, "This is ListFile_Example")
  local files,err = saker.ls(".")
  if files == nil then 
    return false, "ls failed"
  end
  for k,v in pairs(files) 
  do
    if saker.isdir(v) then print(v.." is dir")
    elseif saker.isfile(v) then print(v.." is file")
    else print(v.." is link?")
    end
  end
  return true
end

saker.register("ListFile_Example", "ListFile_Example", PROP_ONCE)