



"use strict"

const Cu = Components.utils; 
const Cc = Components.classes;
const Ci = Components.interfaces;

const EXPORTED_SYMBOLS = ["ObjectWrapper"];



let ObjectWrapper = {
  wrap: function objWrapper_wrap(aObject, aCtxt) {
    let res = Cu.createObjectIn(aCtxt);
    let propList = { };
    for (let prop in aObject) {
      let value;
      if (Array.isArray(aObject[prop])) {
        value = Cu.createArrayIn(aCtxt);
        aObject[prop].forEach(function(aObj) {
          value.push(objWrapper_wrap(aObj, aCtxt));
        });
      } else if (typeof(aObject[prop]) == "object") {
        value = objWrapper_wrap(aObject[prop], aCtxt);
      } else {
        value = aObject[prop];
      }
      propList[prop] = {
        enumerable: true,
        configurable: true,
        writable: true,
        value: value
      }
    }
    Object.defineProperties(res, propList);
    Cu.makeObjectPropsNormal(res);
    return res;
  }
}
