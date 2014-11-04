
function Publish_Example()
  saker.log(LOG_INFO, "This is UUID_Example")
  saker.log(LOG_INFO, "UUID : "..saker.uuid())
  saker.publish("foo", "UUID : "..saker.uuid())
  return True
end

saker.register("Publish_Example", "Publish_Example", PROP_CYCLE)