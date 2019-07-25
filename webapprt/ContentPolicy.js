



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://webapprt/modules/WebappRT.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");


const allowedOrigins = [
  WebappRT.config.app.origin,
  "https://browserid.org",
  "https://www.facebook.com",
  "https://accounts.google.com",
  "https://www.google.com",
  "https://twitter.com",
  "https://api.twitter.com",
];

function ContentPolicy() {}

ContentPolicy.prototype = {
  classID: Components.ID("{75acd178-3d5a-48a7-bd92-fba383520ae6}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentPolicy]),

  shouldLoad: function(contentType, contentLocation, requestOrigin, context, mimeTypeGuess, extra) {
    
    
    let {prePath, scheme} = contentLocation;
    if (contentType == Ci.nsIContentPolicy.TYPE_DOCUMENT &&
        !/^(about|chrome|resource)$/.test(scheme) &&
        allowedOrigins.indexOf(prePath) == -1) {

      
      Cc["@mozilla.org/uriloader/external-protocol-service;1"].
        getService(Ci.nsIExternalProtocolService).
        getProtocolHandlerInfo(scheme).
        launchWithURI(contentLocation);

      
      if (context.currentURI.spec == "about:blank") {
        context.ownerDocument.defaultView.close();
      };

      return Ci.nsIContentPolicy.REJECT_SERVER;
    }

    return Ci.nsIContentPolicy.ACCEPT;
  },

  shouldProcess: function(contentType, contentLocation, requestOrigin, context, mimeType, extra) {
    return Ci.nsIContentPolicy.ACCEPT;
  },
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentPolicy]);
