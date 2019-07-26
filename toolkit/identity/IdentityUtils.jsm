







"use strict";

this.EXPORTED_SYMBOLS = [
  "checkDeprecated",
  "checkRenamed",
  "getRandomId",
  "objectCopy"
];

const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyGetter(this, "logger", function() {
  Cu.import('resource://gre/modules/identity/LogUtils.jsm');
  return getLogger("Identity", "toolkit.identity.debug");
});

function defined(item) {
  return typeof item !== 'undefined';
}

var checkDeprecated = this.checkDeprecated = function checkDeprecated(aOptions, aField) {
  if (defined(aOptions[aField])) {
    logger.log("WARNING: field is deprecated:", aField);
    return true;
  }
  return false;
};

this.checkRenamed = function checkRenamed(aOptions, aOldName, aNewName) {
  if (defined(aOptions[aOldName]) &&
      defined(aOptions[aNewName])) {
    let err = "You cannot provide both " + aOldName + " and " + aNewName;
    logger.error(err);
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





this.objectCopy = function objectCopy(source, target) {
  let desc;
  Object.getOwnPropertyNames(source).forEach(function(name) {
    if (name[0] !== '_') {
      desc = Object.getOwnPropertyDescriptor(source, name);
      Object.defineProperty(target, name, desc);
    }
  });
};
