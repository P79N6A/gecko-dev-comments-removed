






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
const APPLY_IDLE_TIMEOUT =
      Services.prefs.getIntPref("b2g.update.apply-idle-timeout");
const SELF_DESTRUCT_TIMEOUT =
      Services.prefs.getIntPref("b2g.update.self-destruct-timeout");

const APPLY_IDLE_TIMEOUT_SECONDS = APPLY_IDLE_TIMEOUT / 1000;


XPCOMUtils.defineLazyServiceGetter(Services, "aus",
                                   "@mozilla.org/updates/update-service;1",
                                   "nsIApplicationUpdateService");

XPCOMUtils.defineLazyServiceGetter(Services, "idle",
                                   "@mozilla.org/widget/idleservice;1",
                                   "nsIIdleService");
function UpdatePrompt() { }

UpdatePrompt.prototype = {
  classID: Components.ID("{88b3eb21-d072-4e3b-886d-f89d8c49fe59}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdatePrompt,
                                         Ci.nsIRequestObserver,
                                         Ci.nsIProgressEventSink,
                                         Ci.nsIObserver]),

  _update: null,
  _applyPromptTimer: null,
  _waitingForIdle: false,

  

  
  
  
  checkForUpdates: function UP_checkForUpdates() { },

  showUpdateAvailable: function UP_showUpdateAvailable(aUpdate) {
    if (!this.sendUpdateEvent("update-available", aUpdate,
                             this.handleAvailableResult)) {

      log("Unable to prompt for available update, forcing download");
      this.downloadUpdate(aUpdate);
    }
  },

  showUpdateDownloaded: function UP_showUpdateDownloaded(aUpdate, aBackground) {
    
    
    
    
    this.sendUpdateEvent("update-downloaded", aUpdate);

    if (Services.idle.idleTime >= APPLY_IDLE_TIMEOUT) {
      this.showApplyPrompt(aUpdate);
      return;
    }

    
    log("Update is ready to apply, registering idle timeout of " +
        APPLY_IDLE_TIMEOUT_SECONDS + " seconds before prompting.");

    this._update = aUpdate;
    this.waitForIdle();
  },

  showUpdateError: function UP_showUpdateError(aUpdate) {
    if (aUpdate.state == "failed") {
      log("Failed to download update, errorCode: " + aUpdate.errorCode);
    }
  },

  showUpdateHistory: function UP_showUpdateHistory(aParent) { },
  showUpdateInstalled: function UP_showUpdateInstalled() { },

  

  waitForIdle: function UP_waitForIdle() {
    if (this._waitingForIdle) {
      return;
    }

    this._waitingForIdle = true;
    Services.idle.addIdleObserver(this, APPLY_IDLE_TIMEOUT_SECONDS);
    Services.obs.addObserver(this, "quit-application", false);
  },


  showApplyPrompt: function UP_showApplyPrompt(aUpdate) {
    if (!this.sendUpdateEvent("update-prompt-apply", aUpdate,
                             this.handleApplyPromptResult)) {
      log("Unable to prompt, forcing restart");
      this.restartProcess();
      return;
    }

    
    
    this._applyPromptTimer = this.createTimer(APPLY_PROMPT_TIMEOUT);
  },

  sendUpdateEvent: function UP_sendUpdateEvent(aType, aUpdate, aCallback) {
    let detail = {
      displayVersion: aUpdate.displayVersion,
      detailsURL: aUpdate.detailsURL
    };

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

    this._update = aUpdate;
    return this.sendChromeEvent(aType, detail, aCallback);
  },

  sendChromeEvent: function UP_sendChromeEvent(aType, aDetail, aCallback) {
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

    if (!aCallback) {
      browser.shell.sendChromeEvent(detail);
      return true;
    }

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

  handleApplyPromptResult: function UP_handleApplyPromptResult(aDetail) {
    if (this._applyPromptTimer) {
      this._applyPromptTimer.cancel();
      this._applyPromptTimer = null;
    }

    switch (aDetail.result) {
      case "wait":
        
        this.waitForIdle();
        break;
      case "restart":
        this.finishUpdate();
        break;
    }
  },

  downloadUpdate: function UP_downloadUpdate(aUpdate) {
    Services.aus.downloadUpdate(aUpdate, true);
    Services.aus.addDownloadListener(this);
  },

  finishUpdate: function UP_finishUpdate() {
    if (!this._update.isOSUpdate) {
      
      this.restartProcess();
      return;
    }

    let osApplyToDir;
    try {
      this._update.QueryInterface(Ci.nsIWritablePropertyBag);
      osApplyToDir = this._update.getProperty("osApplyToDir");
    } catch (e) {}

    if (!osApplyToDir) {
      log("Error: Update has no osApplyToDir");
      return;
    }

    let updateFile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    updateFile.initWithPath(osApplyToDir + "/update.zip");
    if (!updateFile.exists()) {
      log("Error: FOTA update not found at " + updateFile.path);
      return;
    }

    this.finishOSUpdate(updateFile.path);
  },

  restartProcess: function UP_restartProcess() {
    log("Update downloaded, restarting to apply it");

#ifndef MOZ_WIDGET_GONK
    let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                     .getService(Ci.nsIAppStartup);
    appStartup.quit(appStartup.eForceQuit | appStartup.eRestart);
#else
    
    let pmService = Cc["@mozilla.org/power/powermanagerservice;1"]
                    .getService(Ci.nsIPowerManagerService);
    pmService.restart();
#endif
  },

  finishOSUpdate: function UP_finishOSUpdate(aOsUpdatePath) {
    let recoveryService = Cc["@mozilla.org/recovery-service;1"]
                            .getService(Ci.nsIRecoveryService);

    log("Rebooting into recovery to apply FOTA update: " + aOsUpdatePath);

    try {
      recoveryService.installFotaUpdate(aOsUpdatePath);
    } catch(e) {
      log("Error: Couldn't reboot into recovery to apply FOTA update " +
          aOsUpdatePath);
    }
  },

  

  observe: function UP_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "idle":
        this._waitingForIdle = false;
        this.showApplyPrompt(this._update);
        
      case "quit-application":
        Services.idle.removeIdleObserver(this, APPLY_IDLE_TIMEOUT_SECONDS);
        Services.obs.removeObserver(this, "quit-application");
        break;
    }
  },

  notify: function UP_notify(aTimer) {
    if (aTimer == this._applyPromptTimer) {
      log("Timed out waiting for result, restarting");
      this._applyPromptTimer = null;
      this.finishUpdate();
    }
  },

  createTimer: function UP_createTimer(aTimeoutMs) {
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this, aTimeoutMs, timer.TYPE_ONE_SHOT);
    return timer;
  },

  

  onStartRequest: function UP_onStartRequest(aRequest, aContext) {
    this.sendChromeEvent("update-downloading");
  },

  onStopRequest: function UP_onStopRequest(aRequest, aContext, aStatusCode) {
    Services.aus.removeDownloadListener(this);
  },

  

  onProgress: function UP_onProgress(aRequest, aContext, aProgress,
                                     aProgressMax) {
    this.sendChromeEvent("update-progress", {
      progress: aProgress,
      total: aProgressMax
    });
  },

  onStatus: function UP_onStatus(aRequest, aUpdate, aStatus, aStatusArg) { }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([UpdatePrompt]);
