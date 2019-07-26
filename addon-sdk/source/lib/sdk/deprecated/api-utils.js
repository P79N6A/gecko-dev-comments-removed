





"use strict";

module.metadata = {
  "stability": "deprecated"
};

const memory = require("./memory");

const VALID_TYPES = [
  "array",
  "boolean",
  "function",
  "null",
  "number",
  "object",
  "string",
  "undefined",
];













exports.publicConstructor = function publicConstructor(privateCtor) {
  function PublicCtor() {
    let obj = { constructor: PublicCtor, __proto__: PublicCtor.prototype };
    memory.track(obj, privateCtor.name);
    privateCtor.apply(obj, arguments);
    return obj;
  }
  PublicCtor.prototype = { __proto__: privateCtor.prototype };
  return PublicCtor;
};




































exports.validateOptions = function validateOptions(options, requirements) {
  options = options || {};
  let validatedOptions = {};
  let mapThrew = false;

  for (let key in requirements) {
    let req = requirements[key];
    let [optsVal, keyInOpts] = (key in options) ?
                               [options[key], true] :
                               [undefined, false];
    if (req.map) {
      try {
        optsVal = req.map(optsVal);
      }
      catch (err) {
        mapThrew = true;
      }
    }
    if (req.is) {
      
      req.is.forEach(function (typ) {
        if (VALID_TYPES.indexOf(typ) < 0) {
          let msg = 'Internal error: invalid requirement type "' + typ + '".';
          throw new Error(msg);
        }
      });
      if (req.is.indexOf(getTypeOf(optsVal)) < 0)
        throw requirementError(key, req);
    }
    if (req.ok && !req.ok(optsVal))
      throw requirementError(key, req);

    if (keyInOpts || (req.map && !mapThrew))
      validatedOptions[key] = optsVal;
  }

  return validatedOptions;
};

exports.addIterator = function addIterator(obj, keysValsGenerator) {
  obj.__iterator__ = function(keysOnly, keysVals) {
    let keysValsIterator = keysValsGenerator.call(this);

    
    
    let index = keysOnly ? 0 : 1;
    while (true)
      yield keysVals ? keysValsIterator.next() : keysValsIterator.next()[index];
  };
};



let getTypeOf = exports.getTypeOf = function getTypeOf(val) {
  let typ = typeof(val);
  if (typ === "object") {
    if (!val)
      return "null";
    if (Array.isArray(val))
      return "array";
  }
  return typ;
}


function requirementError(key, requirement) {
  let msg = requirement.msg;
  if (!msg) {
    msg = 'The option "' + key + '" ';
    msg += requirement.is ?
           "must be one of the following types: " + requirement.is.join(", ") :
           "is invalid.";
  }
  return new Error(msg);
}
