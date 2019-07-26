



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

function AppProtocolHandler() {
  this._appInfo = [];
  this._runningInParent = Cc["@mozilla.org/xre/runtime;1"]
                            .getService(Ci.nsIXULRuntime)
                            .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
}

AppProtocolHandler.prototype = {
  classID: Components.ID("{b7ad6144-d344-4687-b2d0-b6b9dce1f07f}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolHandler]),

  scheme: "app",
  defaultPort: -1,
  
  protocolFlags: Ci.nsIProtocolHandler.URI_NOAUTH |
                 Ci.nsIProtocolHandler.URI_DANGEROUS_TO_LOAD |
                 Ci.nsIProtocolHandler.URI_CROSS_ORIGIN_NEEDS_WEBAPPS_PERM,

  getAppInfo: function app_phGetAppInfo(aId) {

    if (!this._appInfo[aId]) {
      this._appInfo[aId] = appsService.getAppInfo(aId);
    }
    return this._appInfo[aId];
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

    
    let appInfo = this.getAppInfo(appId);
    let uri;
    if (this._runningInParent || appInfo.isCoreApp) {
      
      uri = "jar:file://" + appInfo.basePath + appId + "/application.zip!" + fileSpec;
    } else {
      
      uri = "jar:remoteopenfile://" + appInfo.basePath + appId + "/application.zip!" + fileSpec;
    }
    let channel = Services.io.newChannel(uri, null, null);
    channel.QueryInterface(Ci.nsIJARChannel).setAppURI(aURI);
    channel.QueryInterface(Ci.nsIChannel).originalURI = aURI;

    return channel;
  },

  allowPort: function app_phAllowPort(aPort, aScheme) {
    return false;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([AppProtocolHandler]);
