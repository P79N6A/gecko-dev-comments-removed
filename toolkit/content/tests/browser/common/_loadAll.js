








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;
const Cm = Components.manager;


void(function (scriptScope) {
  const kBaseUrl =
    "chrome://mochikit/content/browser/toolkit/content/tests/browser/common/";

  
  var scriptNames = [
    "mockObjects.js",
    "testRunner.js",

    
    "mockFilePicker.js",
    "mockTransferForContinuing.js",
    "toolkitFunctions.js",
  ];

  
  var scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                     getService(Ci.mozIJSSubScriptLoader);
  for (let [, scriptName] in Iterator(scriptNames)) {
    
    
    scriptLoader.loadSubScript(kBaseUrl + scriptName, scriptScope);
  }
}(this));