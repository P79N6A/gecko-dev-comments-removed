







"use strict";

this.EXPORTED_SYMBOLS = [
  "checkDeprecated",
  "checkRenamed",
  "getRandomId",
  "objectCopy",
  "makeMessageObject",
];

const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyModuleGetter(this, "Logger",
                                  "resource://gre/modules/identity/LogUtils.jsm");

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["Identity"].concat(aMessageArgs));
}

function defined(item) {
  return typeof item !== 'undefined';
}

var checkDeprecated = this.checkDeprecated = function checkDeprecated(aOptions, aField) {
  if (defined(aOptions[aField])) {
    log("WARNING: field is deprecated:", aField);
    return true;
  }
  return false;
};

this.checkRenamed = function checkRenamed(aOptions, aOldName, aNewName) {
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

this.getRandomId = function getRandomId() {
  return uuidgen.generateUUID().toString();
};





this.objectCopy = function objectCopy(source, target){
  let desc;
  Object.getOwnPropertyNames(source).forEach(function(name) {
    if (name[0] !== '_') {
      desc = Object.getOwnPropertyDescriptor(source, name);
      Object.defineProperty(target, name, desc);
    }
  });
};

this.makeMessageObject = function makeMessageObject(aRpCaller) {
  let options = {};

  options.id = aRpCaller.id;
  options.origin = aRpCaller.origin;

  
  
  options.loggedInUser = aRpCaller.loggedInUser;

  
  options._internal = aRpCaller._internal;

  Object.keys(aRpCaller).forEach(function(option) {
    
    
    if (!Object.hasOwnProperty(this, option)
        && option[0] !== '_'
        && typeof aRpCaller[option] !== 'function') {
      options[option] = aRpCaller[option];
    }
  });

  
  if ((typeof options.id === 'undefined') ||
      (typeof options.origin === 'undefined')) {
    let err = "id and origin required in relying-party message: " + JSON.stringify(options);
    reportError(err);
    throw new Error(err);
  }

  return options;
}

