







"use strict";

const EXPORTED_SYMBOLS = ["checkRenamed"];
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Logger",
                                  "resource://gre/modules/identity/LogUtils.jsm");

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["Identity"].concat(aMessageArgs));
}

function defined(item) {
  return typeof item !== 'undefined';
}

function checkDeprecated(aOptions, aField) {
  if (defined(aOptions[aField])) {
    log("WARNING: field is deprecated:", aField);
    return true;
  }
  return false;
}

let checkRenamed = function checkRenamed(aOptions, aOldName, aNewName) {
  if (defined(aOptions[aOldName]) &&
      defined(aOptions[aNewName])) {
    let err = "You cannot provide both " + aOldName + " and " + aNewName;
    Logger.reportError(err);
    throw new Error(err);
  }

  if (checkDeprecated(aOptions, aOldName)) {
    aOptions[aNewName] = aOptions[aOldName];
    delete(aOptions[aOldName]);
  }
};