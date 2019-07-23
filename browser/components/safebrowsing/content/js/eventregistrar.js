


















































































function EventRegistrar(eventTypes) {
  this.eventTypes = [];
  this.listeners_ = {};          

  if (eventTypes instanceof Array) {
    var events = eventTypes;
  } else if (typeof eventTypes == "object") {
    var events = [];
    for (var e in eventTypes)
      events.push(eventTypes[e]);
  } else {
    throw new Error("Unrecognized init parameter to EventRegistrar");
  }

  for (var i = 0; i < events.length; i++) {
    this.eventTypes.push(events[i]);          
    this.listeners_[events[i]] = 
      new ListDictionary(events[i] + "Listeners");
  }
}








EventRegistrar.prototype.isKnownEventType = function(eventType) {
  for (var i=0; i < this.eventTypes.length; i++)
    if (eventType == this.eventTypes[i])
      return eventType;
  return false;
}





EventRegistrar.prototype.addEventType = function(eventType) {
  if (this.isKnownEventType(eventType))
    throw new Error("Event type already known: " + eventType);

  this.eventTypes.push(eventType);
  this.listeners_[eventType] = new ListDictionary(eventType + "Listeners");
}







EventRegistrar.prototype.registerListener = function(eventType, listener) {
  if (this.isKnownEventType(eventType) === false)
    throw new Error("Unknown event type: " + eventType);

  this.listeners_[eventType].addMember(listener);
}







EventRegistrar.prototype.removeListener = function(eventType, listener) {
  if (this.isKnownEventType(eventType) === false)
    throw new Error("Unknown event type: " + eventType);

  this.listeners_[eventType].removeMember(listener);
}







EventRegistrar.prototype.fire = function(eventType, e) {
  if (this.isKnownEventType(eventType) === false)
    throw new Error("Unknown event type: " + eventType);

  var invoke = function(listener) {
    listener(e);
  };

  this.listeners_[eventType].forEach(invoke);
}
