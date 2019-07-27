







"use strict";

this.EXPORTED_SYMBOLS = [
  "ObjectUtils"
];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

this.ObjectUtils = {
  











  deepEqual: function(a, b) {
    return _deepEqual(a, b);
  },
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


