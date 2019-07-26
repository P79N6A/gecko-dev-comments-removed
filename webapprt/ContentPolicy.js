









const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function ContentPolicy() {}

ContentPolicy.prototype = {
  classID: Components.ID("{75acd178-3d5a-48a7-bd92-fba383520ae6}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentPolicy]),

  shouldLoad: function(contentType, contentLocation, requestOrigin, context, mimeTypeGuess, extra) {

    return Ci.nsIContentPolicy.ACCEPT;

    
    
    
    
    
  },

  shouldProcess: function(contentType, contentLocation, requestOrigin, context, mimeType, extra) {
    return Ci.nsIContentPolicy.ACCEPT;
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentPolicy]);
