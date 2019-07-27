



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");

this.EXPORTED_SYMBOLS = ["Toast"];



function resolveGeckoURI(uri) {
  if (!uri)
    throw "Can't resolve an empty uri";

  if (uri.startsWith("chrome://")) {
    let registry = Cc['@mozilla.org/chrome/chrome-registry;1'].getService(Ci["nsIChromeRegistry"]);
    return registry.convertChromeURL(Services.io.newURI(uri, null, null)).spec;
  } else if (uri.startsWith("resource://")) {
    let handler = Services.io.getProtocolHandler("resource").QueryInterface(Ci.nsIResProtocolHandler);
    return handler.resolveURI(Services.io.newURI(uri, null, null));
  }

  return uri;
}

var Toast = {
  LONG: "long",
  SHORT: "short",

  show: function(message, duration, options) {
    let msg = {
      type: "Toast:Show",
      message: message,
      duration: duration
    };

    let callback;
    if (options && options.button) {
      msg.button = { };

      
      if (options.button.label) {
        msg.button.label = options.button.label;
      }

      if (options.button.icon) {
        
        
        msg.button.icon = resolveGeckoURI(options.button.icon);
      };

      callback = options.button.callback;
    }

    sendMessageToJava(msg, callback);
  }
}
