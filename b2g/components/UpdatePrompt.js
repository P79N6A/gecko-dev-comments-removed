






const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/WebappsUpdater.jsm");

const VERBOSE = 1;
let log =
  VERBOSE ?
  function log_dump(msg) { dump("UpdatePrompt: "+ msg +"\n"); } :
  function log_noop(msg) { };

const PREF_APPLY_PROMPT_TIMEOUT          = "b2g.update.apply-prompt-timeout";
const PREF_APPLY_IDLE_TIMEOUT            = "b2g.update.apply-idle-timeout";
const PREF_DOWNLOAD_WATCHDOG_TIMEOUT     = "b2g.update.download-watchdog-timeout";
const PREF_DOWNLOAD_WATCHDOG_MAX_RETRIES = "b2g.update.download-watchdog-max-retries";

const NETWORK_ERROR_OFFLINE = 111;
const HTTP_ERROR_OFFSET     = 1000;

const STATE_DOWNLOADING = 'downloading';

XPCOMUtils.defineLazyServiceGetter(Services, "aus",
                                   "@mozilla.org/updates/update-service;1",
                                   "nsIApplicationUpdateService");

XPCOMUtils.defineLazyServiceGetter(Services, "um",
                                   "@mozilla.org/updates/update-manager;1",
                                   "nsIUpdateManager");

XPCOMUtils.defineLazyServiceGetter(Services, "idle",
                                   "@mozilla.org/widget/idleservice;1",
                                   "nsIIdleService");

XPCOMUtils.defineLazyServiceGetter(Services, "settings",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyServiceGetter(Services, 'env',
                                   '@mozilla.org/process/environment;1',
                                   'nsIEnvironment');

function useSettings() {
  
  
  
  
  if (typeof useSettings.result === "undefined") {
    useSettings.result = !Services.env.get("XPCSHELL_TEST_PROFILE_DIR");
  }
  return useSettings.result;
}

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy",
                                  "resource://gre/modules/SystemAppProxy.jsm");

function UpdateCheckListener(updatePrompt) {
  this._updatePrompt = updatePrompt;
}

UpdateCheckListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdateCheckListener]),

  _updatePrompt: null,

  onCheckComplete: function UCL_onCheckComplete(request, updates, updateCount) {
    if (Services.um.activeUpdate) {
      
      
      this._updatePrompt.setUpdateStatus("active-update");
      this._updatePrompt.showUpdateAvailable(Services.um.activeUpdate);
      return;
    }

    if (updateCount == 0) {
      this._updatePrompt.setUpdateStatus("no-updates");
      return;
    }

    let update = Services.aus.selectUpdate(updates, updateCount);
    if (!update) {
      this._updatePrompt.setUpdateStatus("already-latest-version");
      return;
    }

    this._updatePrompt.setUpdateStatus("check-complete");
    this._updatePrompt.showUpdateAvailable(update);
  },

  onError: function UCL_onError(request, update) {
    
    
    let errorCode = update.errorCode >>> 0;
    let isNSError = (errorCode >>> 31) == 1;

    if (errorCode == NETWORK_ERROR_OFFLINE) {
      this._updatePrompt.setUpdateStatus("retry-when-online");
    } else if (isNSError) {
      this._updatePrompt.setUpdateStatus("check-error-" + errorCode);
    } else if (errorCode > HTTP_ERROR_OFFSET) {
      let httpErrorCode = errorCode - HTTP_ERROR_OFFSET;
      this._updatePrompt.setUpdateStatus("check-error-http-" + httpErrorCode);
    }

    Services.aus.QueryInterface(Ci.nsIUpdateCheckListener);
    Services.aus.onError(request, update);
  }
};

function UpdatePrompt() {
  this.wrappedJSObject = this;
  this._updateCheckListener = new UpdateCheckListener(this);
  Services.obs.addObserver(this, "update-check-start", false);
}

UpdatePrompt.prototype = {
  classID: Components.ID("{88b3eb21-d072-4e3b-886d-f89d8c49fe59}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdatePrompt,
                                         Ci.nsIUpdateCheckListener,
                                         Ci.nsIRequestObserver,
                                         Ci.nsIProgressEventSink,
                                         Ci.nsIObserver]),
  _xpcom_factory: XPCOMUtils.generateSingletonFactory(UpdatePrompt),

  _update: null,
  _applyPromptTimer: null,
  _waitingForIdle: false,
  _updateCheckListner: null,

  get applyPromptTimeout() {
    return Services.prefs.getIntPref(PREF_APPLY_PROMPT_TIMEOUT);
  },

  get applyIdleTimeout() {
    return Services.prefs.getIntPref(PREF_APPLY_IDLE_TIMEOUT);
  },

  handleContentStart: function UP_handleContentStart() {
    SystemAppProxy.addEventListener("mozContentEvent", this);
  },

  

  
  
  
  checkForUpdates: function UP_checkForUpdates() { },

  showUpdateAvailable: function UP_showUpdateAvailable(aUpdate) {
    if (!this.sendUpdateEvent("update-available", aUpdate)) {

      log("Unable to prompt for available update, forcing download");
      this.downloadUpdate(aUpdate);
    }
  },

  showUpdateDownloaded: function UP_showUpdateDownloaded(aUpdate, aBackground) {
    
    
    
    
    this.sendUpdateEvent("update-downloaded", aUpdate);

    if (Services.idle.idleTime >= this.applyIdleTimeout) {
      this.showApplyPrompt(aUpdate);
      return;
    }

    let applyIdleTimeoutSeconds = this.applyIdleTimeout / 1000;
    
    log("Update is ready to apply, registering idle timeout of " +
        applyIdleTimeoutSeconds + " seconds before prompting.");

    this._update = aUpdate;
    this.waitForIdle();
  },

  showUpdateError: function UP_showUpdateError(aUpdate) {
    log("Update error, state: " + aUpdate.state + ", errorCode: " +
        aUpdate.errorCode);
    this.sendUpdateEvent("update-error", aUpdate);
    this.setUpdateStatus(aUpdate.statusText);
  },

  showUpdateHistory: function UP_showUpdateHistory(aParent) { },
  showUpdateInstalled: function UP_showUpdateInstalled() {
    if (useSettings()) {
      let lock = Services.settings.createLock();
      lock.set("deviceinfo.last_updated", Date.now(), null, null);
    }
  },

  

  waitForIdle: function UP_waitForIdle() {
    if (this._waitingForIdle) {
      return;
    }

    this._waitingForIdle = true;
    Services.idle.addIdleObserver(this, this.applyIdleTimeout / 1000);
    Services.obs.addObserver(this, "quit-application", false);
  },

  setUpdateStatus: function UP_setUpdateStatus(aStatus) {
     if (useSettings()) {
       log("Setting gecko.updateStatus: " + aStatus);

       let lock = Services.settings.createLock();
       lock.set("gecko.updateStatus", aStatus, null);
     }
  },

  showApplyPrompt: function UP_showApplyPrompt(aUpdate) {
    if (!this.sendUpdateEvent("update-prompt-apply", aUpdate)) {
      log("Unable to prompt, forcing restart");
      this.restartProcess();
      return;
    }

#ifdef MOZ_B2G_RIL
    let window = Services.wm.getMostRecentWindow("navigator:browser");
    let pinReq = window.navigator.mozIccManager.getCardLock("pin");
    pinReq.onsuccess = function(e) {
      if (e.target.result.enabled) {
        
        
        
        
        log("SIM is pin locked. Not starting fallback timer.");
      } else {
        
        
        this._applyPromptTimer = this.createTimer(this.applyPromptTimeout);
      }
    }.bind(this);
    pinReq.onerror = function(e) {
      this._applyPromptTimer = this.createTimer(this.applyPromptTimeout);
    }.bind(this);
#else
    
    
    this._applyPromptTimer = this.createTimer(this.applyPromptTimeout);
#endif
  },

  _copyProperties: ["appVersion", "buildID", "detailsURL", "displayVersion",
                    "errorCode", "isOSUpdate", "platformVersion",
                    "previousAppVersion", "state", "statusText"],

  sendUpdateEvent: function UP_sendUpdateEvent(aType, aUpdate) {
    let detail = {};
    for each (let property in this._copyProperties) {
      detail[property] = aUpdate[property];
    }

    let patch = aUpdate.selectedPatch;
    if (!patch && aUpdate.patchCount > 0) {
      
      
      patch = aUpdate.getPatchAt(0);
    }

    if (patch) {
      detail.size = patch.size;
      detail.updateType = patch.type;
    } else {
      log("Warning: no patches available in update");
    }

    this._update = aUpdate;
    return this.sendChromeEvent(aType, detail);
  },

  sendChromeEvent: function UP_sendChromeEvent(aType, aDetail) {
    let detail = aDetail || {};
    detail.type = aType;

    let sent = SystemAppProxy.dispatchEvent(detail);
    if (!sent) {
      log("Warning: Couldn't send update event " + aType +
          ": no content browser. Will send again when content becomes available.");
      return false;
    }
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
      
      case "low-battery":
        break;
      case "wait":
        
        this.waitForIdle();
        break;
      case "restart":
        this.finishUpdate();
        this._update = null;
        break;
    }
  },

  downloadUpdate: function UP_downloadUpdate(aUpdate) {
    if (!aUpdate) {
      aUpdate = Services.um.activeUpdate;
      if (!aUpdate) {
        log("No active update found to download");
        return;
      }
    }

    let status = Services.aus.downloadUpdate(aUpdate, true);
    if (status == STATE_DOWNLOADING) {
      Services.aus.addDownloadListener(this);
      return;
    }

    
    
    
    if (aUpdate.state == "applied" && aUpdate.errorCode == 0) {
      this.showUpdateDownloaded(aUpdate, true);
      return;
    }

    log("Error downloading update " + aUpdate.name + ": " + aUpdate.errorCode);
    let errorCode = aUpdate.errorCode >>> 0;
    if (errorCode == Cr.NS_ERROR_FILE_TOO_BIG) {
      aUpdate.statusText = "file-too-big";
    }
    this.showUpdateError(aUpdate);
  },

  handleDownloadCancel: function UP_handleDownloadCancel() {
    log("Pausing download");
    Services.aus.pauseDownload();
  },

  finishUpdate: function UP_finishUpdate() {
    if (!this._update.isOSUpdate) {
      
      this.restartProcess();
      return;
    }
 
    try {
      Services.aus.applyOsUpdate(this._update);
    }
    catch (e) {
      this._update.errorCode = Cr.NS_ERROR_FAILURE;
      this.showUpdateError(this._update);
    }
  },

  restartProcess: function UP_restartProcess() {
    log("Update downloaded, restarting to apply it");

    let callbackAfterSet = function() {
#ifndef MOZ_WIDGET_GONK
      let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                       .getService(Ci.nsIAppStartup);
      appStartup.quit(appStartup.eForceQuit | appStartup.eRestart);
#else
      
      let pmService = Cc["@mozilla.org/power/powermanagerservice;1"]
                      .getService(Ci.nsIPowerManagerService);
      pmService.restart();
#endif
    }

    if (useSettings()) {
      
      let lock = Services.settings.createLock({
        handle: callbackAfterSet,
        handleAbort: function(error) {
          log("Abort callback when trying to set previous_os: " + error);
          callbackAfterSet();
        }
      });
      lock.get("deviceinfo.os", {
        handle: function(name, value) {
          log("Set previous_os to: " + value);
          lock.set("deviceinfo.previous_os", value, null, null);
        }
      });
    }
  },

  forceUpdateCheck: function UP_forceUpdateCheck() {
    log("Forcing update check");

    let checker = Cc["@mozilla.org/updates/update-checker;1"]
                    .createInstance(Ci.nsIUpdateChecker);
    checker.checkForUpdates(this._updateCheckListener, true);
  },

  handleEvent: function UP_handleEvent(evt) {
    if (evt.type !== "mozContentEvent") {
      return;
    }

    let detail = evt.detail;
    if (!detail) {
      return;
    }

    switch (detail.type) {
      case "force-update-check":
        this.forceUpdateCheck();
        break;
      case "update-available-result":
        this.handleAvailableResult(detail);
        
        
        
        
        if (this._applyPromptTimer == null && !this._waitingForIdle) {
          this._update = null;
        }
        break;
      case "update-download-cancel":
        this.handleDownloadCancel();
        break;
      case "update-prompt-apply-result":
        this.handleApplyPromptResult(detail);
        break;
    }
  },

  

  observe: function UP_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "idle":
        this._waitingForIdle = false;
        this.showApplyPrompt(this._update);
        
      case "quit-application":
        Services.idle.removeIdleObserver(this, this.applyIdleTimeout / 1000);
        Services.obs.removeObserver(this, "quit-application");
        break;
      case "update-check-start":
        WebappsUpdater.updateApps();
        break;
    }
  },

  

  notify: function UP_notify(aTimer) {
    if (aTimer == this._applyPromptTimer) {
      log("Timed out waiting for result, restarting");
      this._applyPromptTimer = null;
      this.finishUpdate();
      this._update = null;
      return;
    }
    if (aTimer == this._watchdogTimer) {
      log("Download watchdog fired");
      this._watchdogTimer = null;
      this._autoRestartDownload = true;
      Services.aus.pauseDownload();
      return;
    }
  },

  createTimer: function UP_createTimer(aTimeoutMs) {
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this, aTimeoutMs, timer.TYPE_ONE_SHOT);
    return timer;
  },

  

  _startedSent: false,

  _watchdogTimer: null,

  _autoRestartDownload: false,
  _autoRestartCount: 0,

  startWatchdogTimer: function UP_startWatchdogTimer() {
    let watchdogTimeout = 120000;  
    try {
      watchdogTimeout = Services.prefs.getIntPref(PREF_DOWNLOAD_WATCHDOG_TIMEOUT);
    } catch (e) {
      
      
    }
    if (watchdogTimeout <= 0) {
      
      this._watchdogTimer = null;
      return;
    }
    if (this._watchdogTimer) {
      this._watchdogTimer.cancel();
    } else {
      this._watchdogTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }
    this._watchdogTimer.initWithCallback(this, watchdogTimeout,
                                         Ci.nsITimer.TYPE_ONE_SHOT);
  },

  stopWatchdogTimer: function UP_stopWatchdogTimer() {
    if (this._watchdogTimer) {
      this._watchdogTimer.cancel();
      this._watchdogTimer = null;
    }
  },

  touchWatchdogTimer: function UP_touchWatchdogTimer() {
    this.startWatchdogTimer();
  },

  onStartRequest: function UP_onStartRequest(aRequest, aContext) {
    
    
    this._startedSent = false;
    this.startWatchdogTimer();
  },

  onStopRequest: function UP_onStopRequest(aRequest, aContext, aStatusCode) {
    this.stopWatchdogTimer();
    Services.aus.removeDownloadListener(this);
    let paused = !Components.isSuccessCode(aStatusCode);
    if (!paused) {
      
      this._autoRestartDownload = false;
    }
    if (this._autoRestartDownload) {
      this._autoRestartDownload = false;
      let watchdogMaxRetries = Services.prefs.getIntPref(PREF_DOWNLOAD_WATCHDOG_MAX_RETRIES);
      this._autoRestartCount++;
      if (this._autoRestartCount > watchdogMaxRetries) {
        log("Download - retry count exceeded - error");
        
        
        this._autoRestartCount = 0;
        if (Services.um.activeUpdate) {
          this.showUpdateError(Services.um.activeUpdate);
        }
        return;
      }
      log("Download - restarting download - attempt " + this._autoRestartCount);
      this.downloadUpdate(null);
      return;
    }
    this._autoRestartCount = 0;
    this.sendChromeEvent("update-download-stopped", {
      paused: paused
    });
  },

  

  onProgress: function UP_onProgress(aRequest, aContext, aProgress,
                                     aProgressMax) {
    if (aProgress == aProgressMax) {
      
      
      
      this.stopWatchdogTimer();
    } else {
      this.touchWatchdogTimer();
    }
    if (!this._startedSent) {
      this.sendChromeEvent("update-download-started", {
        total: aProgressMax
      });
      this._startedSent = true;
    }

    this.sendChromeEvent("update-download-progress", {
      progress: aProgress,
      total: aProgressMax
    });
  },

  onStatus: function UP_onStatus(aRequest, aUpdate, aStatus, aStatusArg) { }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([UpdatePrompt]);
