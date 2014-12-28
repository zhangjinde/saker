
function pidof_Example()
    local pid, err = saker.pidof("/var/run/nginx.pid", "*nginx*")
    if pid ~= nil then 
      print("nginx pid is "..pid)
    end
    return false, err
end

saker.register("pidof_Example", "pidof_Example", PROP_CYCLE)
