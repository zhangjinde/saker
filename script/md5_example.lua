
function MD5_Example()
    saker.log(LOG_INFO, "This is MD5_Example")
    local teststr = "abcdefghijklmn"
    saker.log(LOG_INFO, "MD5("..teststr..") = "..saker.md5(teststr,string.len(teststr)))
    return true
end

saker.register("MD5_Example", "MD5_Example", PROP_ONCE)
