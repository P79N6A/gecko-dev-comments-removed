



"use strict";




























const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/TelemetryStopwatch.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionFile",
  "resource:///modules/sessionstore/SessionFile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CrashMonitor",
  "resource://gre/modules/CrashMonitor.jsm");

const STATE_RUNNING_STR = "running";


const BROWSER_STARTUP_RESUME_SESSION = 3;

function debug(aMsg) {
  aMsg = ("SessionStartup: " + aMsg).replace(/\S{80}/g, "$&\n");
  Services.console.logStringMessage(aMsg);
}
function warning(aMsg, aException) {
  let consoleMsg = Cc["@mozilla.org/scripterror;1"].createInstance(Ci.nsIScriptError);
consoleMsg.init(aMsg, aException.fileName, null, aException.lineNumber, 0, Ci.nsIScriptError.warningFlag, "component javascript");
  Services.console.logMessage(consoleMsg);
}

let gOnceInitializedDeferred = Promise.defer();



function SessionStartup() {
}

SessionStartup.prototype = {

  
  _initialState: null,
  _sessionType: Ci.nsISessionStartup.NO_SESSION,
  _initialized: false,

  
  _previousSessionCrashed: null,



  


  init: function sss_init() {
    Services.obs.notifyObservers(null, "sessionstore-init-started", null);

    
    if (PrivateBrowsingUtils.permanentPrivateBrowsing) {
      this._initialized = true;
      gOnceInitializedDeferred.resolve();
      return;
    }

    SessionFile.read().then(
      this._onSessionFileRead.bind(this),
      console.error
    );
  },

  
  _createSupportsString: function ssfi_createSupportsString(aData) {
    let string = Cc["@mozilla.org/supports-string;1"]
                   .createInstance(Ci.nsISupportsString);
    string.data = aData;
    return string;
  },

  





  _onSessionFileRead: function ({source, parsed}) {
    this._initialized = true;

    
    let supportsStateString = this._createSupportsString(source);
    Services.obs.notifyObservers(supportsStateString, "sessionstore-state-read", "");
    let stateString = supportsStateString.data;

    if (stateString != source) {
      
      try {
        this._initialState = JSON.parse(stateString);
      } catch (ex) {
        
        
        warning("Observer rewrote the state to something that won't parse", ex);
      }
    } else {
      
      this._initialState = parsed;
    }

    if (this._initialState == null) {
      
      this._sessionType = Ci.nsISessionStartup.NO_SESSION;
      Services.obs.notifyObservers(null, "sessionstore-state-finalized", "");
      gOnceInitializedDeferred.resolve();
      return;
    }

    let shouldResumeSessionOnce = Services.prefs.getBoolPref("browser.sessionstore.resume_session_once");
    let shouldResumeSession = shouldResumeSessionOnce ||
          Services.prefs.getIntPref("browser.startup.page") == BROWSER_STARTUP_RESUME_SESSION;

    
    if (!shouldResumeSessionOnce && this._initialState) {
      delete this._initialState.lastSessionState;
    }

    let resumeFromCrash = Services.prefs.getBoolPref("browser.sessionstore.resume_from_crash");

    CrashMonitor.previousCheckpoints.then(checkpoints => {
      if (checkpoints) {
        
        
        this._previousSessionCrashed = !checkpoints["sessionstore-final-state-write-complete"];
      } else {
        
        
        
        

        if (!this._initialState) {
          
          
          
          this._previousSessionCrashed = false;

        } else {
          
          
          
          
          
          
          
          
          
          let stateFlagPresent = (this._initialState.session &&
                                  this._initialState.session.state);


          this._previousSessionCrashed = !stateFlagPresent ||
            (this._initialState.session.state == STATE_RUNNING_STR);
        }
      }

      
      
      
      Services.telemetry.getHistogramById("SHUTDOWN_OK").add(!this._previousSessionCrashed);

      
      if (this._previousSessionCrashed && resumeFromCrash)
        this._sessionType = Ci.nsISessionStartup.RECOVER_SESSION;
      else if (!this._previousSessionCrashed && shouldResumeSession)
        this._sessionType = Ci.nsISessionStartup.RESUME_SESSION;
      else if (this._initialState)
        this._sessionType = Ci.nsISessionStartup.DEFER_SESSION;
      else
        this._initialState = null; 

      Services.obs.addObserver(this, "sessionstore-windows-restored", true);

      if (this._sessionType != Ci.nsISessionStartup.NO_SESSION)
        Services.obs.addObserver(this, "browser:purge-session-history", true);

      
      Services.obs.notifyObservers(null, "sessionstore-state-finalized", "");
      gOnceInitializedDeferred.resolve();
    });
  },

  


  observe: function sss_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
    case "app-startup":
      Services.obs.addObserver(this, "final-ui-startup", true);
      Services.obs.addObserver(this, "quit-application", true);
      break;
    case "final-ui-startup":
      Services.obs.removeObserver(this, "final-ui-startup");
      Services.obs.removeObserver(this, "quit-application");
      this.init();
      break;
    case "quit-application":
      
      Services.obs.removeObserver(this, "final-ui-startup");
      Services.obs.removeObserver(this, "quit-application");
      if (this._sessionType != Ci.nsISessionStartup.NO_SESSION)
        Services.obs.removeObserver(this, "browser:purge-session-history");
      break;
    case "sessionstore-windows-restored":
      Services.obs.removeObserver(this, "sessionstore-windows-restored");
      
      this._initialState = null;
      break;
    case "browser:purge-session-history":
      Services.obs.removeObserver(this, "browser:purge-session-history");
      
      this._sessionType = Ci.nsISessionStartup.NO_SESSION;
      break;
    }
  },



  get onceInitialized() {
    return gOnceInitializedDeferred.promise;
  },

  


  get state() {
    return this._initialState;
  },

  




  doRestore: function sss_doRestore() {
    return this._willRestore();
  },

  






  isAutomaticRestoreEnabled: function () {
    return Services.prefs.getBoolPref("browser.sessionstore.resume_session_once") ||
           Services.prefs.getIntPref("browser.startup.page") == BROWSER_STARTUP_RESUME_SESSION;
  },

  



  _willRestore: function () {
    return this._sessionType == Ci.nsISessionStartup.RECOVER_SESSION ||
           this._sessionType == Ci.nsISessionStartup.RESUME_SESSION;
  },

  











  get willOverrideHomepage() {
    if (this._initialState && this._willRestore()) {
      let windows = this._initialState.windows || null;
      
      
      return windows && windows.some(w => w.tabs.some(t => !t.pinned));
    }
    return false;
  },

  


  get sessionType() {
    return this._sessionType;
  },

  


  get previousSessionCrashed() {
    return this._previousSessionCrashed;
  },

  
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                          Ci.nsISupportsWeakReference,
                                          Ci.nsISessionStartup]),
  classID:          Components.ID("{ec7a6c20-e081-11da-8ad9-0800200c9a66}")
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SessionStartup]);
