



































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const STATE_RUNNING_STR = "running";
const MAX_FILE_SIZE = 100 * 1024 * 1024; 

function debug(aMsg) {
  aMsg = ("SessionStartup: " + aMsg).replace(/\S{80}/g, "$&\n");
  Services.console.logStringMessage(aMsg);
}



function SessionStartup() {
}

SessionStartup.prototype = {

  
  _initialState: null,
  _sessionType: Ci.nsISessionStartup.NO_SESSION,



  


  init: function sss_init() {
    
    let pbs = Cc["@mozilla.org/privatebrowsing;1"].
              getService(Ci.nsIPrivateBrowsingService);
    if (pbs.autoStarted || pbs.lastChangedByCommandLine)
      return;

    let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefService).getBranch("browser.");

    
    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);
    let sessionFile = dirService.get("ProfD", Ci.nsILocalFile);
    sessionFile.append("sessionstore.js");

    let doResumeSessionOnce = prefBranch.getBoolPref("sessionstore.resume_session_once");
    let doResumeSession = doResumeSessionOnce ||
                          prefBranch.getIntPref("startup.page") == 3;

    
    if (!sessionFile.exists())
      return;

    
    let iniString = this._readStateFile(sessionFile);
    if (!iniString)
      return;

    
    try {
      
      if (iniString.charAt(0) == '(')
        iniString = iniString.slice(1, -1);
      try {
        this._initialState = JSON.parse(iniString);
      }
      catch (exJSON) {
        var s = new Cu.Sandbox("about:blank");
        this._initialState = Cu.evalInSandbox("(" + iniString + ")", s);
      }

      
      if (!doResumeSessionOnce)
        delete this._initialState.lastSessionState;
    }
    catch (ex) { debug("The session file is invalid: " + ex); }

    let resumeFromCrash = prefBranch.getBoolPref("sessionstore.resume_from_crash");
    let lastSessionCrashed =
      this._initialState && this._initialState.session &&
      this._initialState.session.state &&
      this._initialState.session.state == STATE_RUNNING_STR;

    
    
    
    let Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);
    Telemetry.getHistogramById("SHUTDOWN_OK").add(!lastSessionCrashed);

    
    if (lastSessionCrashed && resumeFromCrash)
      this._sessionType = Ci.nsISessionStartup.RECOVER_SESSION;
    else if (!lastSessionCrashed && doResumeSession)
      this._sessionType = Ci.nsISessionStartup.RESUME_SESSION;
    else if (this._initialState)
      this._sessionType = Ci.nsISessionStartup.DEFER_SESSION;
    else
      this._initialState = null; 

    
    
    
    if (this.doRestore() &&
        (!this._initialState.windows ||
        !this._initialState.windows.every(function (win)
           win.tabs.every(function (tab) tab.pinned))))
      Services.obs.addObserver(this, "domwindowopened", true);

    Services.obs.addObserver(this, "sessionstore-windows-restored", true);
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
      break;
    case "domwindowopened":
      var window = aSubject;
      var self = this;
      window.addEventListener("load", function() {
        self._onWindowOpened(window);
        window.removeEventListener("load", arguments.callee, false);
      }, false);
      break;
    case "sessionstore-windows-restored":
      Services.obs.removeObserver(this, "sessionstore-windows-restored");
      
      this._initialState = null;
      this._sessionType = Ci.nsISessionStartup.NO_SESSION;
      break;
    }
  },

  



  _onWindowOpened: function sss_onWindowOpened(aWindow) {
    var wType = aWindow.document.documentElement.getAttribute("windowtype");
    if (wType != "navigator:browser")
      return;
    
    













    var defaultArgs = Cc["@mozilla.org/browser/clh;1"].
                      getService(Ci.nsIBrowserHandler).defaultArgs;
    if (aWindow.arguments && aWindow.arguments[0] &&
        aWindow.arguments[0] == defaultArgs)
      aWindow.arguments[0] = null;

    try {
      Services.obs.removeObserver(this, "domwindowopened");
    } catch (e) {
      
      
    }
  },



  


  get state() {
    return this._initialState;
  },

  



  doRestore: function sss_doRestore() {
    return this._sessionType == Ci.nsISessionStartup.RECOVER_SESSION ||
           this._sessionType == Ci.nsISessionStartup.RESUME_SESSION;
  },

  


  get sessionType() {
    return this._sessionType;
  },



  






  _readStateFile: function sss_readStateFile(aFile) {
    var stateString = Cc["@mozilla.org/supports-string;1"].
                        createInstance(Ci.nsISupportsString);
    stateString.data = this._readFile(aFile) || "";

    Services.obs.notifyObservers(stateString, "sessionstore-state-read", "");

    return stateString.data;
  },

  





  _readFile: function sss_readFile(aFile) {
    try {
      var stream = Cc["@mozilla.org/network/file-input-stream;1"].
                   createInstance(Ci.nsIFileInputStream);
      stream.init(aFile, 0x01, 0, 0);
      var cvstream = Cc["@mozilla.org/intl/converter-input-stream;1"].
                     createInstance(Ci.nsIConverterInputStream);

      var fileSize = stream.available();
      if (fileSize > MAX_FILE_SIZE)
        throw "SessionStartup: sessionstore.js was not processed because it was too large.";

      cvstream.init(stream, "UTF-8", fileSize, Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);
      var data = {};
      cvstream.readString(fileSize, data);
      var content = data.value;
      cvstream.close();

      return content.replace(/\r\n?/g, "\n");
    }
    catch (ex) { Cu.reportError(ex); }

    return null;
  },

  
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                          Ci.nsISupportsWeakReference,
                                          Ci.nsISessionStartup]),
  classID:          Components.ID("{ec7a6c20-e081-11da-8ad9-0800200c9a66}"),
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([SessionStartup]);
