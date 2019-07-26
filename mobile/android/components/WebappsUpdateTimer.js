







"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/WebappManager.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function log(message) {
  
  
  
  
  
  dump(message);
}

function WebappsUpdateTimer() {}

WebappsUpdateTimer.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback,
                                         Ci.nsISupportsWeakReference]),
  classID: Components.ID("{8f7002cb-e959-4f0a-a2e8-563232564385}"),

  notify: function(aTimer) {
    if (Services.prefs.getIntPref("browser.webapps.checkForUpdates") == 0) {
      
      log("Webapps update timer invoked in webapp process; ignoring.");
      return;
    }

    
    if (Services.io.offline) {
      log("network offline for webapp update check; waiting");
      Services.obs.addObserver(this, "network:offline-status-changed", true);
      return;
    }

    log("periodic check for webapp updates");
    WebappManager.checkForUpdates();
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic !== "network:offline-status-changed" || aData !== "online") {
      return;
    }

    log("network back online for webapp update check; commencing");
    Services.obs.removeObserver(this, "network:offline-status-changed");
    WebappManager.checkForUpdates();
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([WebappsUpdateTimer]);
