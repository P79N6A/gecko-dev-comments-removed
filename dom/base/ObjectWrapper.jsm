



"use strict"

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = ["ObjectWrapper"];



const TypedArrayThings = [
  "Int8Array",
  "Uint8Array",
  "Uint8ClampedArray",
  "Int16Array",
  "Uint16Array",
  "Int32Array",
  "Uint32Array",
  "Float32Array",
  "Float64Array",
];

this.ObjectWrapper = {
  getObjectKind: function objWrapper_getObjectKind(aObject) {
    if (aObject === null || aObject === undefined) {
      return "primitive";
    } else if (Array.isArray(aObject)) {
      return "array";
    } else if (aObject instanceof Ci.nsIDOMFile) {
      return "file";
    } else if (aObject instanceof Ci.nsIDOMBlob) {
      return "blob";
    } else if (aObject instanceof Date) {
      return "date";
    } else if (TypedArrayThings.indexOf(aObject.constructor.name) !== -1) {
      return aObject.constructor.name;
    } else if (typeof aObject == "object") {
      return "object";
    } else {
      return "primitive";
    }
  },

  wrap: function objWrapper_wrap(aObject, aCtxt) {
    dump("-*- ObjectWrapper is deprecated. Use Components.utils.cloneInto() instead.\n");
    return Cu.cloneInto(aObject, aCtxt, { cloneFunctions: true });
  }
}
