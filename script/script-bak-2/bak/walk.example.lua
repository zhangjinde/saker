
function Walk_Example()
  saker.log(LOG_INFO, "This is Walk_Example")
  local files = saker.walk("..");
  for k,v in pairs(files) do
     saker.log(LOG_INFO, v)
  end
  return True
end

saker.register("Walk_Example", "Walk_Example", PROP_ONCE)