



"use strict"

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

const EXPORTED_SYMBOLS = ["ObjectWrapper"];



let ObjectWrapper = {
  getObjectKind: function objWrapper_getobjectkind(aObject) {
    if (!aObject) {
      return "null";
    }

    if (Array.isArray(aObject)) {
      return "array";
    } else if (aObject.mozSlice && (typeof aObject.mozSlice == "function")) {
      return "blob";
    } else if (typeof aObject == "object") {
      return "object";
    } else {
      return "primitive";
    }
  },

  wrap: function objWrapper_wrap(aObject, aCtxt) {
    if (!aObject) {
      return null;
    }

    
    let kind = this.getObjectKind(aObject);
    if (kind == "array") {
      let res = Cu.createArrayIn(aCtxt);
      aObject.forEach(function(aObj) {
        res.push(this.wrap(aObj, aCtxt));
      }, this);
      return res;
    } else if (kind == "blob") {
      return new aCtxt.Blob([aObject]);
    } else if (kind == "primitive") {
      return aObject;
    }

    
    let res = Cu.createObjectIn(aCtxt);
    let propList = { };
    for (let prop in aObject) {
      let value;
      let objProp = aObject[prop];
      let propKind = this.getObjectKind(objProp);
      if (propKind == "array") {
        value = Cu.createArrayIn(aCtxt);
        objProp.forEach(function(aObj) {
          value.push(this.wrap(aObj, aCtxt));
        }, this);
      } else if (propKind == "blob") {
        value = new aCtxt.Blob([objProp]);
      } else if (propKind == "object") {
        value = this.wrap(objProp, aCtxt);
      } else {
        value = objProp;
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
