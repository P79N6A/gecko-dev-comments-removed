



"use strict";









this.EXPORTED_SYMBOLS = [ "DevToolsUtils" ];

Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Components.interfaces.mozIJSSubScriptLoader)
  .loadSubScript("resource://gre/modules/devtools/DevToolsUtils.js", this);

this.DevToolsUtils = {
  safeErrorString: safeErrorString,
  reportException: reportException,
  makeInfallible: makeInfallible,
  yieldingEach: yieldingEach,
  reportingDisabled: false , 
  defineLazyPrototypeGetter: defineLazyPrototypeGetter
};
