





"use strict";

this.EXPORTED_SYMBOLS = [
  "Promise"
];


















































































this.Cc = Components.classes;
this.Ci = Components.interfaces;
this.Cu = Components.utils;
this.Cr = Components.results;

this.Cc["@mozilla.org/moz/jssubscript-loader;1"]
    .getService(this.Ci.mozIJSSubScriptLoader)
    .loadSubScript("resource://gre/modules/Promise-backend.js", this);
