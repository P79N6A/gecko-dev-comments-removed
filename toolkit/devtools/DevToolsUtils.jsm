



"use strict";



this.EXPORTED_SYMBOLS = [ "DevToolsUtils" ];

var Cu = Components.utils;


function safeErrorString(aError) {
  try {
    var s = aError.toString();
    if (typeof s === "string")
      return s;
  } catch (ee) { }

  return "<failed trying to find error description>";
}




function reportException(aWho, aException) {
  let msg = aWho + " threw an exception: " + safeErrorString(aException);
  if (aException.stack) {
    msg += "\nCall stack:\n" + aException.stack;
  }

  dump(msg + "\n");

  if (Cu.reportError) {
    




    Cu.reportError(msg);
  }
}















function makeInfallible(aHandler, aName) {
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

this.DevToolsUtils = {
  safeErrorString: safeErrorString,
  reportException: reportException,
  makeInfallible: makeInfallible
};
