


function Actor() {}

exports.register = function(handle) {
  handle.addGlobalActor(Actor, "registeredActor2");
  handle.addTabActor(Actor, "registeredActor2");
}

exports.unregister = function(handle) {
  handle.removeTabActor(Actor);
  handle.removeGlobalActor(Actor);
}

