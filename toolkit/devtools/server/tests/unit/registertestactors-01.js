


function Actor() {}

exports.register = function(handle) {
  handle.addTabActor(Actor, "registeredActor1");
  handle.addGlobalActor(Actor, "registeredActor1");
}

exports.unregister = function(handle) {
  handle.removeTabActor(Actor);
  handle.removeGlobalActor(Actor);
}

