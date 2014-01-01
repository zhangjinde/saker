
function Linux_adoptExample()
  saker.log(LOG_INFO, "this is Linux_ExecExample")
  local cmd=saker.pwd().."/../script/app.sh"
  local pidfile=saker.pwd().."/../pid/app.sh.pid"
  
  local pid,err = saker.pidof(pidfile)

  if not pid then
    local pid,retval = saker.adopt(nil, cmd)
    if pid then
        saker.log(LOG_INFO, "pid is "..pid.." pidfile is "..retval)
    else
        saker.log(LOG_ERROR, retval)
    end
  else
    saker.log(LOG_INFO, "pid has been running pid:"..pid)
  end

  return true
end

function Linux_DeferKillExample()
  saker.log(LOG_INFO, "this is Linux_DeferKillExample")
  local cmd=saker.pwd().."/../script/app.sh"
  local pidfile=saker.pwd().."/../pid/app.sh.pid"
  local pid,err = saker.pidof(pidfile)
  if pid then
     saker.kill(pid)
     saker.log(LOG_INFO, "kill -9 pid "..pid)
  end
  return true
end


function Win_adoptExample()
  saker.log(LOG_INFO, "this is Win_ExecExample")
  
  local cmd="calc a b" 
  local pidfile=saker.pwd().."/../pid/calc.pid"
  local pid,err = saker.pidof(pidfile)

  if not pid then
    local pid,err = saker.adopt(nil, cmd)
    if pid then
	    saker.log(LOG_INFO, "pid is "..pid)
    else
     	saker.log(LOG_ERROR, err)
    end
  else
    saker.log(LOG_INFO, "pid has been running pid:"..pid)
  end

  return true
end

function Win_DeferKillExample()
  saker.log(LOG_INFO, "this is Win_DeferKillExample")

  local cmd="calc" 
  local pidfile=saker.pwd().."/../pid/calc.pid"
  local pid,err = saker.pidof(pidfile)
  if pid then
     saker.kill(pid)
     saker.log(LOG_INFO, "kill -9 pid "..pid)
  end
  return true
end



if saker.uname() == "WIN" then
	saker.register("Win_adoptExample", "Win_adoptExample", PROP_CYCLE)
  saker.register("Win_DeferKillExample", "Win_DeferKillExample", PROP_DEFER)
else
  os.execute("chmod +x "..saker.pwd().."/../script/app.sh")
	saker.register("Linux_adoptExample", "Linux_adoptExample", PROP_CYCLE)
  saker.register("Linux_DeferKillExample", "Linux_DeferKillExample", PROP_DEFER)
end

