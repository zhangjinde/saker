
local jsontable ={
['foo']='bar',
['xoo']='mor'
}

proc_table={}
proc_table['calc']={
	['pid']  = '../pid/calc.pid',
	['cmd']  = 'calc',
	['param']= '-a -c',
	['check']= 'yes'
}

proc_table['system32']={
	['pid']  = '../pid/test2.pid',
	['cmd']  = 'ses',
	['param']= '/c',
	['check']= 'no'
}

function Json_Example()
  saker.log(LOG_INFO, "This is Json_Example")
  
  saker.log(LOG_INFO, "encode double table json string: \n"..json.encode(proc_table))
  
  saker.log(LOG_INFO, "after encode")
  jsonstr = json.encode(jsontable)
  
  saker.log(LOG_INFO, "after decode")
  local myt = json.decode(jsonstr)
  for k,v in pairs(myt)
	do
    saker.log(LOG_INFO, k..":"..v)
  end
  
  -- "[123,456]"
  local jsontable = {123} 
  --arraystr = json.encode(jsontable);
  --local arrayobj = json.decode(arraystr) 
  
  saker.log(LOG_INFO, json.encode(jsontable))
  
  return True
end
    

saker.register("Json_Example", "Json_Example", PROP_ONCE)
