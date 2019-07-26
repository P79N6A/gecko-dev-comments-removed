




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
  "LoginManagerContent", "resource://gre/modules/LoginManagerContent.jsm");
XPCOMUtils.defineLazyModuleGetter(this,
  "InsecurePasswordUtils", "resource://gre/modules/InsecurePasswordUtils.jsm");



this.__defineGetter__("webNavigation", function () {
  return docShell.QueryInterface(Ci.nsIWebNavigation);
});

addMessageListener("WebNavigation:LoadURI", function (message) {
  let flags = message.json.flags || webNavigation.LOAD_FLAGS_NONE;

  webNavigation.loadURI(message.json.uri, flags, null, null, null);
});

addMessageListener("Browser:HideSessionRestoreButton", function (message) {
  
  let doc = content.document;
  let container;
  if (doc.documentURI.toLowerCase() == "about:home" &&
      (container = doc.getElementById("sessionRestoreContainer"))){
    container.hidden = true;
  }
});

if (!Services.prefs.getBoolPref("browser.tabs.remote")) {
  addEventListener("DOMContentLoaded", function(event) {
    LoginManagerContent.onContentLoaded(event);
  });
  addEventListener("DOMFormHasPassword", function(event) {
    InsecurePasswordUtils.checkForInsecurePasswords(event.target);
  });
  addEventListener("DOMAutoComplete", function(event) {
    LoginManagerContent.onUsernameInput(event);
  });
  addEventListener("blur", function(event) {
    LoginManagerContent.onUsernameInput(event);
  });
}
