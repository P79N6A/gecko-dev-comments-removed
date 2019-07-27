


"use strict";

module.metadata = {
  "stability": "unstable"
};





const MAX_INT = 0x7FFFFFFF;
const MIN_INT = -0x80000000;

const {Cc,Ci,Cr} = require("chrome");

const prefService = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefService);
const prefSvc = prefService.getBranch(null);
const defaultBranch = prefService.getDefaultBranch(null);

const { Preferences } = require("resource://gre/modules/Preferences.jsm");
const prefs = new Preferences({});

const branchKeys = branchName =>
  keys(branchName).map($ => $.replace(branchName, ""));

const Branch = function(branchName) {
  return new Proxy(Branch.prototype, {
    getOwnPropertyDescriptor(target, name, receiver) {
      return {
        configurable: true,
        enumerable: true,
        writable: false,
        value: this.get(target, name, receiver)
      };
    },
    enumerate(target) {
      return branchKeys(branchName)[Symbol.iterator]();
    },
    ownKeys(target) {
      return branchKeys(branchName);
    },
    get(target, name, receiver) {
      return get(`${branchName}${name}`);
    },
    set(target, name, value, receiver) {
      set(`${branchName}${name}`, value);
    },
    has(target, name) {
      return this.hasOwn(target, name);
    },
    hasOwn(target, name) {
      return has(`${branchName}${name}`);
    },
    deleteProperty(target, name) {
      reset(`${branchName}${name}`);
      return true;
    }
  });
}


function get(name, defaultValue) {
  return prefs.get(name, defaultValue);
}
exports.get = get;


function set(name, value) {
  var prefType;
  if (typeof value != "undefined" && value != null)
    prefType = value.constructor.name;

  switch (prefType) {
  case "Number":
    if (value % 1 != 0)
      throw new Error("cannot store non-integer number: " + value);
  }

  prefs.set(name, value);
}
exports.set = set;

const has = prefs.has.bind(prefs)
exports.has = has;

function keys(root) {
  return prefSvc.getChildList(root);
}
exports.keys = keys;

const isSet = prefs.isSet.bind(prefs);
exports.isSet = isSet;

function reset(name) {
  try {
    prefSvc.clearUserPref(name);
  }
  catch (e) {
    
    
    
    
    
    
    
    if (e.result != Cr.NS_ERROR_UNEXPECTED) {
      throw e;
    }
  }
}
exports.reset = reset;

function getLocalized(name, defaultValue) {
  let value = null;
  try {
    value = prefSvc.getComplexValue(name, Ci.nsIPrefLocalizedString).data;
  }
  finally {
    return value || defaultValue;
  }
}
exports.getLocalized = getLocalized;

function setLocalized(name, value) {
  
  
  
  
  
  defaultBranch.setCharPref(name, value);
}
exports.setLocalized = setLocalized;

exports.Branch = Branch;

