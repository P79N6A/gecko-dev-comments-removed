






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

  _selfDestructTimer: null,

  

  
  
  
  checkForUpdates: function UP_checkForUpdates() { },
  showUpdateAvailable: function UP_showUpdateAvailable(aUpdate) { },

  showUpdateDownloaded: function UP_showUpdateDownloaded(aUpdate, aBackground) {
    
    
    

    log("Update downloaded, restarting to apply it");

    
    
    this._setSelfDestructTimer(5000);

    let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
    
    
    
    
    appStartup.quit(appStartup.eForceQuit
#ifndef ANDROID
                    | appStartup.eRestart
#endif
      );
  },

  _setSelfDestructTimer: function UP__setSelfDestructTimer(timeoutMs) {
#ifdef ANDROID
    Cu.import("resource://gre/modules/ctypes.jsm");
    let libc = ctypes.open("libc.so");
    let _exit = libc.declare("_exit",  ctypes.default_abi,
                             ctypes.void_t, 
                             ctypes.int);   
    this.notify = function UP_notify(_) {
      log("Self-destruct timer fired; didn't cleanly shut down.  BOOM");
      _exit(0);
    }

    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this, timeoutMs, timer.TYPE_ONE_SHOT);
    this._selfDestructTimer = timer;
#endif
  },

  showUpdateInstalled: function UP_showUpdateInstalled() { },

  showUpdateError: function UP_showUpdateError(aUpdate) {
    if (aUpdate.state == "failed") {
      log("Failed to download update, errorCode: " + aUpdate.errorCode);
    }
  },

  showUpdateHistory: function UP_showUpdateHistory(aParent) { },
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([UpdatePrompt]);
