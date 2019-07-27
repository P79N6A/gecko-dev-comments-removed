



"use strict";

this.EXPORTED_SYMBOLS = ["Local"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/stringbundle.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://gre/modules/Preferences.jsm");

function lazyStrings(name) {
  let bundle = "chrome://weave/locale/services/" + name + ".properties";
  return () => new StringBundle(bundle);
}

this.Str = {};
XPCOMUtils.defineLazyGetter(Str, "errors", lazyStrings("errors"));
XPCOMUtils.defineLazyGetter(Str, "sync", lazyStrings("sync"));

function makeGUID() {
  return CommonUtils.encodeBase64URL(CryptoUtils.generateRandomBytes(9));
}

this.Local = function () {
  let prefs = new Preferences("services.cloudsync.");
  this.__defineGetter__("prefs", function () {
    return prefs;
  });
};

Local.prototype = {
  get id() {
    let clientId = this.prefs.get("client.GUID", "");
    return clientId == "" ? this.id = makeGUID(): clientId;
  },

  set id(value) {
    this.prefs.set("client.GUID", value);
  },

  get name() {
    let clientName = this.prefs.get("client.name", "");

    if (clientName != "") {
      return clientName;
    }

    
    let env = Cc["@mozilla.org/process/environment;1"]
                .getService(Ci.nsIEnvironment);
    let user = env.get("USER") || env.get("USERNAME");
    let appName;
    let brand = new StringBundle("chrome://branding/locale/brand.properties");
    let brandName = brand.get("brandShortName");

    try {
      let syncStrings = new StringBundle("chrome://browser/locale/sync.properties");
      appName = syncStrings.getFormattedString("sync.defaultAccountApplication", [brandName]);
    } catch (ex) {
    }

    appName = appName || brandName;

    let system =
      
      Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2).get("device") ||
      
      Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2).get("host") ||
      
      Cc["@mozilla.org/network/protocol;1?name=http"].getService(Ci.nsIHttpProtocolHandler).oscpu;

    return this.name = Str.sync.get("client.name2", [user, appName, system]);
  },

  set name(value) {
    this.prefs.set("client.name", value);
  },
};

