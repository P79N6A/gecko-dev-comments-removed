







"use strict";

this.EXPORTED_SYMBOLS = [
  "ObjectUtils"
];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);


XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");

this.ObjectUtils = {
  











  deepEqual: function(a, b) {
    return _deepEqual(a, b);
  },

  














  strict: function(obj) {
    return _strict(obj);
  }
};






function _deepEqual(a, b) {
  

  
  if (a === b) {
    return true;
  
  
  } else if (instanceOf(a, "Date") && instanceOf(b, "Date")) {
    if (isNaN(a.getTime()) && isNaN(b.getTime()))
      return true;
    return a.getTime() === b.getTime();
  
  
  
  } else if (instanceOf(a, "RegExp") && instanceOf(b, "RegExp")) {
    return a.source === b.source &&
           a.global === b.global &&
           a.multiline === b.multiline &&
           a.lastIndex === b.lastIndex &&
           a.ignoreCase === b.ignoreCase;
  
  
  } else if (typeof a != "object" && typeof b != "object") {
    return a == b;
  
  
  
  
  
  
  } else {
    return objEquiv(a, b);
  }
}

function instanceOf(object, type) {
  return Object.prototype.toString.call(object) == "[object " + type + "]";
}

function isUndefinedOrNull(value) {
  return value === null || value === undefined;
}

function isArguments(object) {
  return instanceOf(object, "Arguments");
}

function objEquiv(a, b) {
  if (isUndefinedOrNull(a) || isUndefinedOrNull(b)) {
    return false;
  }
  
  if (a.prototype !== b.prototype) {
    return false;
  }
  
  
  if (isArguments(a)) {
    if (!isArguments(b)) {
      return false;
    }
    a = pSlice.call(a);
    b = pSlice.call(b);
    return _deepEqual(a, b);
  }
  let ka, kb;
  try {
    ka = Object.keys(a);
    kb = Object.keys(b);
  } catch (e) {
    
    return false;
  }
  
  
  if (ka.length != kb.length)
    return false;
  
  ka.sort();
  kb.sort();
  
  
  for (let key of ka) {
    if (!_deepEqual(a[key], b[key])) {
      return false;
    }
  }
  return true;
}



function _strict(obj) {
  if (typeof obj != "object") {
    throw new TypeError("Expected an object");
  }

  return new Proxy(obj, {
    get: function(target, name) {
      if (name in obj) {
        return obj[name];
      }

      let error = new TypeError(`No such property: "${name}"`);
      Promise.reject(error); 
      throw error;
    }
  });
}
