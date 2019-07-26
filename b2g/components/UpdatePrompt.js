






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

const APPLY_PROMPT_TIMEOUT =
      Services.prefs.getIntPref("b2g.update.apply-prompt-timeout");
const APPLY_WAIT_TIMEOUT =
      Services.prefs.getIntPref("b2g.update.apply-wait-timeout");
const SELF_DESTRUCT_TIMEOUT =
      Services.prefs.getIntPref("b2g.update.self-destruct-timeout");

XPCOMUtils.defineLazyServiceGetter(Services, "aus",
                                   "@mozilla.org/updates/update-service;1",
                                   "nsIApplicationUpdateService");

function UpdatePrompt() { }

UpdatePrompt.prototype = {
  classID: Components.ID("{88b3eb21-d072-4e3b-886d-f89d8c49fe59}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdatePrompt]),

  _update: null,
  _applyPromptTimer: null,
  _applyWaitTimer: null,
  _selfDestructTimer: null,

  

  
  
  
  checkForUpdates: function UP_checkForUpdates() { },

  showUpdateAvailable: function UP_showUpdateAvailable(aUpdate) {
    if (!this.sendUpdateEvent("update-available", aUpdate,
                             this.handleAvailableResult)) {

      log("Unable to prompt for available update, forcing download");
      this.downloadUpdate(aUpdate);
    }
  },

  showUpdateDownloaded: function UP_showUpdateDownloaded(aUpdate, aBackground) {
    if (!this.sendUpdateEvent("update-downloaded", aUpdate,
                             this.handleDownloadedResult)) {
      log("Unable to prompt, forcing restart");
      this.restartProcess();
      return;
    }

    
    
    this._applyPromptTimer = this.createTimer(APPLY_PROMPT_TIMEOUT);
  },

  sendUpdateEvent: function UP_sendUpdateEvent(aType, aUpdate, aCallback,
                                               aDetail) {
    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    if (!browser) {
      log("Warning: Couldn't send update event " + aType +
          ": no content browser");
      return false;
    }

    let content = browser.getContentWindow();
    if (!content) {
      log("Warning: Couldn't send update event " + aType +
          ": no content window");
      return false;
    }

    let detail = aDetail || {};
    detail.type = aType;
    detail.displayVersion = aUpdate.displayVersion;
    detail.detailsURL = aUpdate.detailsURL;

    let patch = aUpdate.selectedPatch;
    if (!patch) {
      
      
      if (aUpdate.patchCount == 0) {
        log("Warning: no patches available in update");
        return false;
      }

      patch = aUpdate.getPatchAt(0);
    }

    detail.size = patch.size;
    detail.updateType = patch.type;

    if (!aCallback) {
      browser.shell.sendChromeEvent(detail);
      return true;
    }

    this._update = aUpdate;
    let resultType = aType + "-result";

    let handleContentEvent = (function(e) {
      if (!e.detail) {
        return;
      }

      let detail = e.detail;
      if (detail.type == resultType) {
        aCallback.call(this, detail);
        content.removeEventListener("mozContentEvent", handleContentEvent);
        this._update = null;
      }
    }).bind(this);

    content.addEventListener("mozContentEvent", handleContentEvent);
    browser.shell.sendChromeEvent(detail);
    return true;
  },

  handleAvailableResult: function UP_handleAvailableResult(aDetail) {
    
    
    switch (aDetail.result) {
      case "download":
        this.downloadUpdate(this._update);
        break;
    }
  },

  handleDownloadedResult: function UP_handleDownloadedResult(aDetail) {
    if (this._applyPromptTimer) {
      this._applyPromptTimer.cancel();
      this._applyPromptTimer = null;
    }

    switch (aDetail.result) {
      case "wait":
        
        
        this._applyWaitTimer = this.createTimer(APPLY_WAIT_TIMEOUT);
        break;
      case "restart":
        this.restartProcess();
        break;
    }
  },

  downloadUpdate: function UP_downloadUpdate(aUpdate) {
    Services.aus.downloadUpdate(aUpdate, true);
  },

  restartProcess: function UP_restartProcess() {
    log("Update downloaded, restarting to apply it");

    
    
    this._selfDestructTimer = this.createTimer(SELF_DESTRUCT_TIMEOUT);

    let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                       .getService(Ci.nsIAppStartup);
    
    
    
    
    appStartup.quit(appStartup.eForceQuit
#ifndef ANDROID
                    | appStartup.eRestart
#endif
      );
  },

  notify: function UP_notify(aTimer) {
    if (aTimer == this._selfDestructTimer) {
      this._selfDestructTimer = null;
      this.selfDestruct();
    } else if (aTimer == this._applyPromptTimer) {
      log("Timed out waiting for result, restarting");
      this._applyPromptTimer = null;
      this.restartProcess();
    } else if (aTimer == this._applyWaitTimer) {
      this._applyWaitTimer = null;
      this.showUpdatePrompt();
    }
  },

  selfDestruct: function UP_selfDestruct() {
#ifdef ANDROID
    Cu.import("resource://gre/modules/ctypes.jsm");
    let libc = ctypes.open("libc.so");
    let _exit = libc.declare("_exit", ctypes.default_abi,
                             ctypes.void_t, 
                             ctypes.int);   

    log("Self-destruct timer fired; didn't cleanly shut down.  BOOM");
    _exit(0);
#endif
  },

  createTimer: function UP_createTimer(aTimeoutMs) {
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this, aTimeoutMs, timer.TYPE_ONE_SHOT);
    return timer;
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
