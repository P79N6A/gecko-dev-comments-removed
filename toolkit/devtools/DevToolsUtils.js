



"use strict";






this.safeErrorString = function safeErrorString(aError) {
  try {
    let errorString = aError.toString();
    if (typeof errorString === "string") {
      
      
      try {
        if (aError.stack) {
          let stack = aError.stack.toString();
          if (typeof stack === "string") {
            errorString += "\nStack: " + stack;
          }
        }
      } catch (ee) { }

      return errorString;
    }
  } catch (ee) { }

  return "<failed trying to find error description>";
}




this.reportException = function reportException(aWho, aException) {
  let msg = aWho + " threw an exception: " + safeErrorString(aException);

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
