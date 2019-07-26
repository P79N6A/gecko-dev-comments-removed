



"use strict";




this.safeErrorString = function safeErrorString(aError) {
  try {
    var s = aError.toString();
    if (typeof s === "string")
      return s;
  } catch (ee) { }

  return "<failed trying to find error description>";
}




this.reportException = function reportException(aWho, aException) {
  let msg = aWho + " threw an exception: " + safeErrorString(aException);
  if (aException.stack) {
    msg += "\nCall stack:\n" + aException.stack;
  }

  dump(msg + "\n");

  if (Components.utils.reportError) {
    




    Components.utils.reportError(msg);
  }
}















this.makeInfallible = function makeInfallible(aHandler, aName) {
  if (!aName)
    aName = aHandler.name;

  return function () {
    try {
      return aHandler.apply(this, arguments);
    } catch (ex) {
      let who = "Handler function";
      if (aName) {
        who += " " + aName;
      }
      reportException(who, ex);
    }
  }
}
