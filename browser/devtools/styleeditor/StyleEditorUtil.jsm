




"use strict";

this.EXPORTED_SYMBOLS = [
  "_",
  "assert",
  "log",
  "text",
  "wire",
  "showFilePicker"
];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

const PROPERTIES_URL = "chrome://browser/locale/devtools/styleeditor.properties";

const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const console = require("resource://gre/modules/devtools/Console.jsm").console;
const gStringBundle = Services.strings.createBundle(PROPERTIES_URL);










this._ = function _(aName)
{
  try {
    if (arguments.length == 1) {
      return gStringBundle.GetStringFromName(aName);
    }
    let rest = Array.prototype.slice.call(arguments, 1);
    return gStringBundle.formatStringFromName(aName, rest, rest.length);
  }
  catch (ex) {
    console.error(ex);
    throw new Error("L10N error. '" + aName + "' is missing from " + PROPERTIES_URL);
  }
}









this.assert = function assert(aExpression, aMessage)
{
  if (!!!(aExpression)) {
    let msg = aMessage ? "ASSERTION FAILURE:" + aMessage : "ASSERTION FAILURE";
    log(msg);
    throw new Error(msg);
  }
  return aExpression;
}














this.text = function text(aRoot, aSelector, aText)
{
  let element = aRoot.querySelector(aSelector);
  if (!element) {
    return null;
  }

  if (aText === undefined) {
    return element.textContent;
  }
  element.textContent = aText;
  return aText;
}








function forEach(aObject, aCallback)
{
  for (let key in aObject) {
    if (aObject.hasOwnProperty(key)) {
      aCallback(key, aObject[key]);
    }
  }
}








this.log = function log()
{
  console.logStringMessage(Array.prototype.slice.call(arguments).join(" "));
}


















this.wire = function wire(aRoot, aSelectorOrElement, aDescriptor)
{
  let matches;
  if (typeof(aSelectorOrElement) == "string") { 
    matches = aRoot.querySelectorAll(aSelectorOrElement);
    if (!matches.length) {
      return;
    }
  } else {
    matches = [aSelectorOrElement]; 
  }

  if (typeof(aDescriptor) == "function") {
    aDescriptor = {events: {click: aDescriptor}};
  }

  for (let i = 0; i < matches.length; i++) {
    let element = matches[i];
    forEach(aDescriptor.events, function (aName, aHandler) {
      element.addEventListener(aName, aHandler, false);
    });
    forEach(aDescriptor.attributes, element.setAttribute);
  }
}

















this.showFilePicker = function showFilePicker(path, toSave, parentWindow,
                                              callback, suggestedFilename)
{
  if (typeof(path) == "string") {
    try {
      if (Services.io.extractScheme(path) == "file") {
        let uri = Services.io.newURI(path, null, null);
        let file = uri.QueryInterface(Ci.nsIFileURL).file;
        callback(file);
        return;
      }
    } catch (ex) {
      callback(null);
      return;
    }
    try {
      let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      file.initWithPath(path);
      callback(file);
      return;
    } catch (ex) {
      callback(null);
      return;
    }
  }
  if (path) { 
    callback(path);
    return;
  }

  let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
  let mode = toSave ? fp.modeSave : fp.modeOpen;
  let key = toSave ? "saveStyleSheet" : "importStyleSheet";
  let fpCallback = function(result) {
    if (result == Ci.nsIFilePicker.returnCancel) {
      callback(null);
    } else {
      callback(fp.file);
    }
  };

  if (toSave && suggestedFilename) {
    fp.defaultString = suggestedFilename;
  }

  fp.init(parentWindow, _(key + ".title"), mode);
  fp.appendFilters(_(key + ".filter"), "*.css");
  fp.appendFilters(fp.filterAll);
  fp.open(fpCallback);
  return;
}
