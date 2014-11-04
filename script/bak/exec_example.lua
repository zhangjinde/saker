
function Linux_ExecExample()
  saker.log(LOG_INFO, "this is Linux_ExecExample")
  local cmd=saker.pwd().."/../script/app.sh"
  local pidfile=saker.pwd().."/../pid/app.sh.pid"
  
  local pid,err = saker.pidof(pidfile)

  if not pid then
    pid,retval = saker.exec(nil, cmd)
    if pid then
        saker.log(LOG_INFO, "pid is "..pid.." pidfile is "..retval)
    else
        saker.log(LOG_ERROR, retval)
    end
  else
    saker.log(LOG_INFO, "pid has been running pid:"..pid)
  end

  return True
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
  return True
end


function Win_ExecExample()
  saker.log(LOG_INFO, "this is Win_ExecExample")
  
  local cmd="calc" 
  local pidfile=saker.pwd().."/../pid/calc.pid"
  local pid,err = saker.pidof(pidfile)

  if not pid then
    pid,err = saker.exec(nil, cmd)
    if pid then
	    saker.log(LOG_INFO, "pid is "..pid)
    else
     	saker.log(LOG_ERROR, err)
    end
  else
    saker.log(LOG_INFO, "pid has been running pid:"..pid)
  end

  return True
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
  return True
end



if saker.uname() == "WIN" then
	saker.register("Win_ExecExample", "Win_ExecExample", PROP_CYCLE)
  saker.register("Win_DeferKillExample", "Win_DeferKillExample", PROP_DEFER)
else
  os.execute("chmod +x app.sh")
	saker.register("Linux_ExecExample", "Linux_ExecExample", PROP_CYCLE)
  saker.register("Linux_DeferKillExample", "Linux_DeferKillExample", PROP_DEFER)
end

