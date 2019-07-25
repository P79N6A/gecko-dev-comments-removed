




































"use strict";

const EXPORTED_SYMBOLS = [
  "_",
  "assert",
  "attr",
  "getCurrentBrowserTabContentWindow",
  "log",
  "text",
  "wire"
];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

const PROPERTIES_URL = "chrome://browser/locale/devtools/styleeditor.properties";

const console = Services.console;
const gStringBundle = Services.strings.createBundle(PROPERTIES_URL);










function _(aName)
{

  if (arguments.length == 1) {
    return gStringBundle.GetStringFromName(aName);
  }
  let rest = Array.prototype.slice.call(arguments, 1);
  return gStringBundle.formatStringFromName(aName, rest, rest.length);
}









function assert(aExpression, aMessage)
{
  if (!!!(aExpression)) {
    let msg = aMessage ? "ASSERTION FAILURE:" + aMessage : "ASSERTION FAILURE";
    log(msg);
    throw new Error(msg);
  }
  return aExpression;
}














function text(aRoot, aSelector, aText)
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








function log()
{
  console.logStringMessage(Array.prototype.slice.call(arguments).join(" "));
}



















function wire(aRoot, aSelectorOrElement, aDescriptor)
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

  for (let i = 0; i < matches.length; ++i) {
    let element = matches[i];
    forEach(aDescriptor.events, function (aName, aHandler) {
      element.addEventListener(aName, aHandler, false);
    });
    forEach(aDescriptor.attributes, element.setAttribute);
    forEach(aDescriptor.userData, element.setUserData);
  }
}

