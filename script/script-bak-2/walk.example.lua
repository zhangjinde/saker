
function Walk_Example()
  saker.log(LOG_INFO, "This is Walk_Example")
  local files = saker.walk("..", "*.exe")
  for k,v in pairs(files) do
     saker.log(LOG_INFO, v)
  end
  return true
end

saker.register("Walk_Example", "Walk_Example", PROP_ONCE)