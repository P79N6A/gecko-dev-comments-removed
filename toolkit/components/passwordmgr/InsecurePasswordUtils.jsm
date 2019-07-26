



this.EXPORTED_SYMBOLS = [ "InsecurePasswordUtils" ];

const Ci = Components.interfaces;
const Cu = Components.utils;
const Cc = Components.classes;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "devtools",
                                  "resource://gre/modules/devtools/Loader.jsm");

Object.defineProperty(this, "WebConsoleUtils", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/utils").Utils;
  },
  configurable: true,
  enumerable: true
});

const STRINGS_URI = "chrome://global/locale/security/security.properties";
let l10n = new WebConsoleUtils.l10n(STRINGS_URI);

this.InsecurePasswordUtils = {

  _sendWebConsoleMessage : function (messageTag, domDoc) {
    





    let  windowId = WebConsoleUtils.getInnerWindowId(domDoc.defaultView);
    let category = "Insecure Password Field";
    let flag = Ci.nsIScriptError.warningFlag;
    let message = l10n.getStr(messageTag);
    let consoleMsg = Cc["@mozilla.org/scripterror;1"]
      .createInstance(Ci.nsIScriptError);

    consoleMsg.initWithWindowID(
      message, "", 0, 0, 0, flag, category, windowId);

    Services.console.logMessage(consoleMsg);
  },

  



























  _checkIfURIisSecure : function(uri) {
    let isSafe = false;
    let netutil = Cc["@mozilla.org/network/util;1"].getService(Ci.nsINetUtil);
    let ph = Ci.nsIProtocolHandler;

    if (netutil.URIChainHasFlags(uri, ph.URI_IS_LOCAL_RESOURCE) ||
        netutil.URIChainHasFlags(uri, ph.URI_DOES_NOT_RETURN_DATA) ||
        netutil.URIChainHasFlags(uri, ph.URI_INHERITS_SECURITY_CONTEXT) ||
        netutil.URIChainHasFlags(uri, ph.URI_SAFE_TO_LOAD_IN_SECURE_CONTEXT)) {

      isSafe = true;
    }

    return isSafe;
  },

  











  _checkForInsecureNestedDocuments : function(domDoc) {
    let uri = domDoc.documentURIObject;
    if (domDoc.defaultView == domDoc.defaultView.parent) {
      
      return false;
    }
    if (!this._checkIfURIisSecure(uri)) {
      
      return true;
    }
    
    return this._checkForInsecureNestedDocuments(domDoc.defaultView.parent.document);
  },


  





  checkForInsecurePasswords : function (aForm) {
    var domDoc = aForm.ownerDocument;
    let pageURI = domDoc.defaultView.top.document.documentURIObject;
    let isSafePage = this._checkIfURIisSecure(pageURI);

    if (!isSafePage) {
      this._sendWebConsoleMessage("InsecurePasswordsPresentOnPage", domDoc);
    }

    
    
    if (this._checkForInsecureNestedDocuments(domDoc)) {
      this._sendWebConsoleMessage("InsecurePasswordsPresentOnIframe", domDoc);
    }

    if (aForm.action.match(/^http:\/\//)) {
      this._sendWebConsoleMessage("InsecureFormActionPasswordsPresent", domDoc);
    }
  },
};
