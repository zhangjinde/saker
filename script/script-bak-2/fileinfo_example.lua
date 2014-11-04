


local fileinfo,err = saker.fileinfo(saker.pwd().."/../build.sh", true)
if fileinfo == nil then
  print(err)
else 
  for k,v in pairs(fileinfo) do
      if type(v) == "boolean" then 
         if v then print (k.." : true") 
         else print(k.." : false") end
      else 
         print(k.."  :" ..v)
      end
      
  end
  local format = "%d.%m.%Y %H:%M:%S"
  print(os.date(format, fileinfo["MTIME"]))
end

