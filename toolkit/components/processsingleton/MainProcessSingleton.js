



"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyServiceGetter(this, "globalmm",
                                   "@mozilla.org/globalmessagemanager;1",
                                   "nsIMessageBroadcaster");

function MainProcessSingleton() {}
MainProcessSingleton.prototype = {
  classID: Components.ID("{0636a680-45cb-11e4-916c-0800200c9a66}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  receiveMessage: function(message) {
    let logMsg = message.data;
    logMsg.wrappedJSObject = logMsg;
    Services.obs.notifyObservers(logMsg, "console-api-log-event", null);
  },

  observe: function(subject, topic, data) {
    switch (topic) {
    case "app-startup": {
      Services.obs.addObserver(this, "xpcom-shutdown", false);

      
      
      globalmm.loadFrameScript("chrome://global/content/browser-content.js", true);
      ppmm.addMessageListener("Console:Log", this);
      break;
    }

    case "xpcom-shutdown":
      ppmm.removeMessageListener("Console:Log", this);
      break;
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MainProcessSingleton]);
