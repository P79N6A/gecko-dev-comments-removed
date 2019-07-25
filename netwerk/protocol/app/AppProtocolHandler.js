



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsISyncMessageSender");

function AppProtocolHandler() {
  this._basePath = null;
}

AppProtocolHandler.prototype = {
  classID: Components.ID("{b7ad6144-d344-4687-b2d0-b6b9dce1f07f}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolHandler]),

  scheme: "app",
  defaultPort: -1,
  
  protocolFlags2: Ci.nsIProtocolHandler.URI_NORELATIVE |
                  Ci.nsIProtocolHandler.URI_NOAUTH |
                  Ci.nsIProtocolHandler.URI_LOADABLE_BY_ANYONE,

  get basePath() {
    if (!this._basePath) {
      this._basePath = cpmm.sendSyncMessage("Webapps:GetBasePath", { })[0] + "/";
    }

    return this._basePath;
  },

  newURI: function app_phNewURI(aSpec, aOriginCharset, aBaseURI) {
    let uri = Cc["@mozilla.org/network/standard-url;1"]
              .createInstance(Ci.nsIStandardURL);
    uri.init(Ci.nsIStandardURL.URLTYPE_STANDARD, -1, aSpec, aOriginCharset,
             aBaseURI);
    return uri.QueryInterface(Ci.nsIURI);
  },

  newChannel: function app_phNewChannel(aURI) {
    
    
    let noScheme = aURI.spec.substring(6);
    let firstSlash = noScheme.indexOf("/");

    let appId = noScheme;
    let fileSpec = aURI.path;

    if (firstSlash) {
      appId = noScheme.substring(0, firstSlash);
    }

    
    
    let lastSlash = fileSpec.lastIndexOf("/");
    if (lastSlash == fileSpec.length - 1) {
      fileSpec += "index.html";
    } else if (fileSpec[lastSlash + 1] == '#') {
      let anchor = fileSpec.substring(lastSlash + 1);
      fileSpec = fileSpec.substring(0, lastSlash) + "/index.html" + anchor;
    }

    
    let uri = "jar:file://" + this.basePath + appId + "/application.zip!" + fileSpec;
    let channel = Services.io.newChannel(uri, null, null);
    channel.QueryInterface(Ci.nsIJARChannel).setAppURI(aURI);
    channel.QueryInterface(Ci.nsIChannel).originalURI = aURI;

    return channel;
  },

  allowPort: function app_phAllowPort(aPort, aScheme) {
    return false;
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([AppProtocolHandler]);
