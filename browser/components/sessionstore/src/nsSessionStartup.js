



































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const STATE_RUNNING_STR = "running";
const MAX_FILE_SIZE = 100 * 1024 * 1024; 

XPCOMUtils.defineLazyServiceGetter(this, "ConsoleSvc",
  "@mozilla.org/consoleservice;1", "nsIConsoleService");

XPCOMUtils.defineLazyServiceGetter(this, "ObserverSvc",
  "@mozilla.org/observer-service;1", "nsIObserverService");

function debug(aMsg) {
  aMsg = ("SessionStartup: " + aMsg).replace(/\S{80}/g, "$&\n");
  ConsoleSvc.logStringMessage(aMsg);
}



function SessionStartup() {
}

SessionStartup.prototype = {

  
  _iniString: null,
  _sessionType: Ci.nsISessionStartup.NO_SESSION,



  


  init: function sss_init() {
    
    let pbs = Cc["@mozilla.org/privatebrowsing;1"].
              getService(Ci.nsIPrivateBrowsingService);
    if (pbs.autoStarted)
      return;

    let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefService).getBranch("browser.");

    
    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);
    let sessionFile = dirService.get("ProfD", Ci.nsILocalFile);
    sessionFile.append("sessionstore.js");
    
    let doResumeSession = prefBranch.getBoolPref("sessionstore.resume_session_once") ||
                          prefBranch.getIntPref("startup.page") == 3;
    
    
    var resumeFromCrash = prefBranch.getBoolPref("sessionstore.resume_from_crash");
    if (!resumeFromCrash && !doResumeSession || !sessionFile.exists())
      return;
    
    
    this._iniString = this._readStateFile(sessionFile);
    if (!this._iniString)
      return;
    
    try {
      
      var s = new Cu.Sandbox("about:blank");
      var initialState = Cu.evalInSandbox("(" + this._iniString + ")", s);
    }
    catch (ex) { debug("The session file is invalid: " + ex); } 
    
    let lastSessionCrashed =
      initialState && initialState.session && initialState.session.state &&
      initialState.session.state == STATE_RUNNING_STR;
    
    
    if (lastSessionCrashed && resumeFromCrash)
      this._sessionType = Ci.nsISessionStartup.RECOVER_SESSION;
    else if (!lastSessionCrashed && doResumeSession)
      this._sessionType = Ci.nsISessionStartup.RESUME_SESSION;
    else
      this._iniString = null; 

    if (this._sessionType != Ci.nsISessionStartup.NO_SESSION) {
      
      ObserverSvc.addObserver(this, "domwindowopened", true);
      ObserverSvc.addObserver(this, "browser:purge-session-history", true);
    }
  },

  


  observe: function sss_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
    case "app-startup": 
      ObserverSvc.addObserver(this, "final-ui-startup", true);
      ObserverSvc.addObserver(this, "quit-application", true);
      break;
    case "final-ui-startup": 
      ObserverSvc.removeObserver(this, "final-ui-startup");
      ObserverSvc.removeObserver(this, "quit-application");
      this.init();
      break;
    case "quit-application":
      
      ObserverSvc.removeObserver(this, "final-ui-startup");
      ObserverSvc.removeObserver(this, "quit-application");
      break;
    case "domwindowopened":
      var window = aSubject;
      var self = this;
      window.addEventListener("load", function() {
        self._onWindowOpened(window);
        window.removeEventListener("load", arguments.callee, false);
      }, false);
      break;
    case "browser:purge-session-history":
      
      this._iniString = null;
      this._sessionType = Ci.nsISessionStartup.NO_SESSION;
      
      ObserverSvc.removeObserver(this, "browser:purge-session-history");
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

    ObserverSvc.removeObserver(this, "domwindowopened");
  },



  


  get state() {
    return this._iniString;
  },

  



  doRestore: function sss_doRestore() {
    return this._sessionType != Ci.nsISessionStartup.NO_SESSION;
  },

  


  get sessionType() {
    return this._sessionType;
  },



  






  _readStateFile: function sss_readStateFile(aFile) {
    var stateString = Cc["@mozilla.org/supports-string;1"].
                        createInstance(Ci.nsISupportsString);
    stateString.data = this._readFile(aFile) || "";

    ObserverSvc.notifyObservers(stateString, "sessionstore-state-read", "");

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
  classDescription: "Browser Session Startup Service",
  classID:          Components.ID("{ec7a6c20-e081-11da-8ad9-0800200c9a66}"),
  contractID:       "@mozilla.org/browser/sessionstartup;1",

  
  _xpcom_categories: [
    
    { category: "app-startup", service: true }
  ]

};

function NSGetModule(aCompMgr, aFileSpec)
  XPCOMUtils.generateModule([SessionStartup]);
