
-- saker退出时执行
function Defer_Example()
    saker.log(LOG_INFO, "This is Defer_Example")
    return true
end

saker.register("Defer_Example", "Defer_Example", PROP_DEFER)
