



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

XPCOMUtils.defineLazyModuleGetter(this, "_SessionFile",
  "resource:///modules/sessionstore/_SessionFile.jsm");

const STATE_RUNNING_STR = "running";

function debug(aMsg) {
  aMsg = ("SessionStartup: " + aMsg).replace(/\S{80}/g, "$&\n");
  Services.console.logStringMessage(aMsg);
}

let gOnceInitializedDeferred = Promise.defer();



function SessionStartup() {
}

SessionStartup.prototype = {

  
  _initialState: null,
  _sessionType: Ci.nsISessionStartup.NO_SESSION,
  _initialized: false,



  


  init: function sss_init() {
    
    if (PrivateBrowsingUtils.permanentPrivateBrowsing) {
      this._initialized = true;
      gOnceInitializedDeferred.resolve();
      return;
    }

    _SessionFile.read().then(
      this._onSessionFileRead.bind(this),
      Cu.reportError
    );
  },

  
  _createSupportsString: function ssfi_createSupportsString(aData) {
    let string = Cc["@mozilla.org/supports-string;1"]
                   .createInstance(Ci.nsISupportsString);
    string.data = aData;
    return string;
  },

  _onSessionFileRead: function sss_onSessionFileRead(aStateString) {
    if (this._initialized) {
      
      return;
    }
    try {
      this._initialized = true;

      
      let supportsStateString = this._createSupportsString(aStateString);
      Services.obs.notifyObservers(supportsStateString, "sessionstore-state-read", "");
      aStateString = supportsStateString.data;

      
      if (!aStateString) {
        this._sessionType = Ci.nsISessionStartup.NO_SESSION;
        return;
      }

      
      
      if (aStateString.charAt(0) == '(')
        aStateString = aStateString.slice(1, -1);
      let corruptFile = false;
      try {
        this._initialState = JSON.parse(aStateString);
      }
      catch (ex) {
        debug("The session file contained un-parse-able JSON: " + ex);
        
        
        
        try {
          var s = new Cu.Sandbox("about:blank", {sandboxName: 'nsSessionStartup'});
          this._initialState = Cu.evalInSandbox("(" + aStateString + ")", s);
        } catch(ex) {
          debug("The session file contained un-eval-able JSON: " + ex);
          corruptFile = true;
        }
      }
      Services.telemetry.getHistogramById("FX_SESSION_RESTORE_CORRUPT_FILE").add(corruptFile);

      let doResumeSessionOnce = Services.prefs.getBoolPref("browser.sessionstore.resume_session_once");
      let doResumeSession = doResumeSessionOnce ||
            Services.prefs.getIntPref("browser.startup.page") == 3;

      
      if (!doResumeSessionOnce)
        delete this._initialState.lastSessionState;

      let resumeFromCrash = Services.prefs.getBoolPref("browser.sessionstore.resume_from_crash");
      let lastSessionCrashed =
        this._initialState && this._initialState.session &&
        this._initialState.session.state &&
        this._initialState.session.state == STATE_RUNNING_STR;

      
      
      
      Services.telemetry.getHistogramById("SHUTDOWN_OK").add(!lastSessionCrashed);

      
      if (lastSessionCrashed && resumeFromCrash)
        this._sessionType = Ci.nsISessionStartup.RECOVER_SESSION;
      else if (!lastSessionCrashed && doResumeSession)
        this._sessionType = Ci.nsISessionStartup.RESUME_SESSION;
      else if (this._initialState)
        this._sessionType = Ci.nsISessionStartup.DEFER_SESSION;
      else
        this._initialState = null; 

      Services.obs.addObserver(this, "sessionstore-windows-restored", true);

      if (this._sessionType != Ci.nsISessionStartup.NO_SESSION)
        Services.obs.addObserver(this, "browser:purge-session-history", true);

    } finally {
      
      Services.obs.notifyObservers(null, "sessionstore-state-finalized", "");
      gOnceInitializedDeferred.resolve();
    }
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
    this._ensureInitialized();
    return this._initialState;
  },

  





  doRestore: function sss_doRestore() {
    this._ensureInitialized();
    return this._willRestore();
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
    this._ensureInitialized();
    return this._sessionType;
  },

  
  
  
  _ensureInitialized: function sss__ensureInitialized() {
    try {
      if (this._initialized) {
        
        return;
      }
      let contents = _SessionFile.syncRead();
      this._onSessionFileRead(contents);
    } catch(ex) {
      debug("ensureInitialized: could not read session " + ex + ", " + ex.stack);
      throw ex;
    }
  },

  
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                          Ci.nsISupportsWeakReference,
                                          Ci.nsISessionStartup]),
  classID:          Components.ID("{ec7a6c20-e081-11da-8ad9-0800200c9a66}")
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SessionStartup]);
