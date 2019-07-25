






const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const VERBOSE = 1;
let log =
  VERBOSE ?
  function log_dump(msg) { dump("UpdatePrompt: "+ msg +"\n"); } :
  function log_noop(msg) { };

function UpdatePrompt() { }

UpdatePrompt.prototype = {
  classID: Components.ID("{88b3eb21-d072-4e3b-886d-f89d8c49fe59}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdatePrompt]),

  

  
  
  
  checkForUpdates: function UP_checkForUpdates() { },
  showUpdateAvailable: function UP_showUpdateAvailable(aUpdate) { },

  showUpdateDownloaded: function UP_showUpdateDownloaded(aUpdate, aBackground) {
    
    
    

    log("Update downloaded, restarting to apply it");

    let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
    
    
    
    
    appStartup.quit(appStartup.eForceQuit
#ifndef ANDROID
                    | appStartup.eRestart
#endif
      );
  },

  showUpdateInstalled: function UP_showUpdateInstalled() { },

  showUpdateError: function UP_showUpdateError(aUpdate) {
    if (aUpdate.state == "failed") {
      log("Failed to download update");
    }
  },

  showUpdateHistory: function UP_showUpdateHistory(aParent) { },
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([UpdatePrompt]);
