



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "cpmm", function() {
  return Cc["@mozilla.org/childprocessmessagemanager;1"]
           .getService(Ci.nsIMessageSender);
});

function YoutubeProtocolHandler() {
}

YoutubeProtocolHandler.prototype = {
  classID: Components.ID("{c3f1b945-7e71-49c8-95c7-5ae9cc9e2bad}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolHandler]),

  scheme: "vnd.youtube",
  defaultPort: -1,
  protocolFlags: Ci.nsIProtocolHandler.URI_NORELATIVE |
                 Ci.nsIProtocolHandler.URI_NOAUTH |
                 Ci.nsIProtocolHandler.URI_LOADABLE_BY_ANYONE,

  
  
  
  newURI: function yt_phNewURI(aSpec, aOriginCharset, aBaseURI) {
    let uri = Cc["@mozilla.org/network/standard-url;1"]
              .createInstance(Ci.nsIStandardURL);
    uri.init(Ci.nsIStandardURL.URLTYPE_NO_AUTHORITY, this.defaultPort,
             aSpec, aOriginCharset, aBaseURI);
    return uri.QueryInterface(Ci.nsIURI);
  },

  newChannel: function yt_phNewChannel(aURI) {
    






    cpmm.sendAsyncMessage("content-handler", {
      type: 'video/youtube', 
      url: aURI.spec         
    });

    throw Components.results.NS_ERROR_ILLEGAL_VALUE;
  },

  allowPort: function yt_phAllowPort(aPort, aScheme) {
    return false;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([YoutubeProtocolHandler]);
