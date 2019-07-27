



"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

function ContentProcessSingleton() {}
ContentProcessSingleton.prototype = {
  classID: Components.ID("{ca2a8470-45c7-11e4-916c-0800200c9a66}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  observe: function(subject, topic, data) {
    switch (topic) {
    case "app-startup": {
      Services.obs.addObserver(this, "console-api-log-event", false);
      Services.obs.addObserver(this, "xpcom-shutdown", false);
      cpmm.addMessageListener("DevTools:InitDebuggerServer", this);
      break;
    }
    case "console-api-log-event": {
      let consoleMsg = subject.wrappedJSObject;

      let msgData = {
        level: consoleMsg.level,
        filename: consoleMsg.filename,
        lineNumber: consoleMsg.lineNumber,
        functionName: consoleMsg.functionName,
        timeStamp: consoleMsg.timeStamp,
        arguments: [],
      };

      
      
      for (let arg of consoleMsg.arguments) {
        if ((typeof arg == "object" || typeof arg == "function") && arg !== null) {
          msgData.arguments.push("<unavailable>");
        } else {
          msgData.arguments.push(arg);
        }
      }

      cpmm.sendAsyncMessage("Console:Log", msgData);
      break;
    }

    case "xpcom-shutdown":
      Services.obs.removeObserver(this, "console-api-log-event");
      Services.obs.removeObserver(this, "xpcom-shutdown");
      cpmm.removeMessageListener("DevTools:InitDebuggerServer", this);
      break;
    }
  },

  receiveMessage: function (message) {
    
    
    if (Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT) {
      let {init} = Cu.import("resource://gre/modules/devtools/content-server.jsm", {});
      init(message);
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentProcessSingleton]);
