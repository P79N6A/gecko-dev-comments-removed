


































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const STATE_RUNNING_STR = "running";

function debug(aMsg) {
  aMsg = ("SessionStartup: " + aMsg).replace(/\S{80}/g, "$&\n");
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService)
                                     .logStringMessage(aMsg);
}



function SessionStartup() {
}

SessionStartup.prototype = {

  
  _iniString: null,
  _sessionType: Ci.nsISessionStartup.NO_SESSION,



  


  init: function sss_init() {
    this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefService).getBranch("browser.");

    
    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);
    this._sessionFile = dirService.get("ProfD", Ci.nsILocalFile);
    this._sessionFile.append("sessionstore.js");
    
    
    var resumeFromCrash = this._prefBranch.getBoolPref("sessionstore.resume_from_crash");
    if ((resumeFromCrash || this._doResumeSession()) && this._sessionFile.exists()) {
      
      this._iniString = this._readStateFile(this._sessionFile);
      if (this._iniString) {
        try {
          
          var s = new Components.utils.Sandbox("about:blank");
          var initialState = Components.utils.evalInSandbox(this._iniString, s);

          
          this._lastSessionCrashed =
            initialState.session && initialState.session.state &&
            initialState.session.state == STATE_RUNNING_STR;
        
        }
        catch (ex) { debug("The session file is invalid: " + ex); } 
      }
    }

    
    if (this._iniString) {
      if (this._lastSessionCrashed && this._doRecoverSession())
        this._sessionType = Ci.nsISessionStartup.RECOVER_SESSION;
      else if (!this._lastSessionCrashed && this._doResumeSession())
        this._sessionType = Ci.nsISessionStartup.RESUME_SESSION;
      else
        this._iniString = null; 
    }

    if (this._sessionType != Ci.nsISessionStartup.NO_SESSION) {
      
      var observerService = Cc["@mozilla.org/observer-service;1"].
                            getService(Ci.nsIObserverService);
      observerService.addObserver(this, "domwindowopened", true);
    }
  },

  


  observe: function sss_observe(aSubject, aTopic, aData) {
    var observerService = Cc["@mozilla.org/observer-service;1"].
                          getService(Ci.nsIObserverService);

    switch (aTopic) {
    case "app-startup": 
      observerService.addObserver(this, "final-ui-startup", true);
      observerService.addObserver(this, "quit-application", true);
      break;
    case "final-ui-startup": 
      observerService.removeObserver(this, "final-ui-startup");
      observerService.removeObserver(this, "quit-application");
      this.init();
      break;
    case "quit-application":
      
      observerService.removeObserver(this, "final-ui-startup");
      observerService.removeObserver(this, "quit-application");
      break;
    case "domwindowopened":
      var window = aSubject;
      var self = this;
      window.addEventListener("load", function() {
        self._onWindowOpened(window);
        window.removeEventListener("load", arguments.callee, false);
      }, false);
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
    
    var observerService = Cc["@mozilla.org/observer-service;1"].
                          getService(Ci.nsIObserverService);
    observerService.removeObserver(this, "domwindowopened");
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



  



  _doResumeSession: function sss_doResumeSession() {
    return this._prefBranch.getIntPref("startup.page") == 3 || 
      this._prefBranch.getBoolPref("sessionstore.resume_session_once");
  },

  




  _doRecoverSession: function sss_doRecoverSession() {
    
    if (!this._prefBranch.getBoolPref("sessionstore.resume_from_crash"))
      return false;

    
    var recover = true;

    
    
    var dialogURI = null;
    try {
      dialogURI = this._prefBranch.getCharPref("sessionstore.restore_prompt_uri");
    }
    catch (ex) { }
    
    try {
      if (dialogURI) { 
        var params = Cc["@mozilla.org/embedcomp/dialogparam;1"].
                     createInstance(Ci.nsIDialogParamBlock);
        
        params.SetInt(0, 0);
        Cc["@mozilla.org/embedcomp/window-watcher;1"].
        getService(Ci.nsIWindowWatcher).
        openWindow(null, dialogURI, "_blank", 
                   "chrome,modal,centerscreen,titlebar", params);
        recover = params.GetInt(0) == 0;
      }
      else { 
        
        const brandShortName = this._getStringBundle("chrome://branding/locale/brand.properties")
                                   .GetStringFromName("brandShortName");
        
        var ssStringBundle = this._getStringBundle("chrome://browser/locale/sessionstore.properties");
        var restoreTitle = ssStringBundle.formatStringFromName("restoredTitle", [brandShortName], 1);
        var restoreText = ssStringBundle.formatStringFromName("restoredMsg", [brandShortName], 1);
        var okTitle = ssStringBundle.GetStringFromName("okTitle");
        var cancelTitle = ssStringBundle.GetStringFromName("cancelTitle");

        var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                            getService(Ci.nsIPromptService);
        
        var flags = promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0 +
                    promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_1 +
                    promptService.BUTTON_POS_0_DEFAULT;
        var buttonChoice = promptService.confirmEx(null, restoreTitle, restoreText, 
                                          flags, okTitle, cancelTitle, null, 
                                          null, {});
        recover = (buttonChoice == 0);
      }
    }
    catch (ex) { dump(ex + "\n"); } 
    return recover;
  },

  




  _getStringBundle: function sss_getStringBundle(aURI) {
    return Cc["@mozilla.org/intl/stringbundle;1"].
           getService(Ci.nsIStringBundleService).createBundle(aURI);
  },



  






  _readStateFile: function sss_readStateFile(aFile) {
    var stateString = Cc["@mozilla.org/supports-string;1"].
                        createInstance(Ci.nsISupportsString);
    stateString.data = this._readFile(aFile) || "";
    
    var observerService = Cc["@mozilla.org/observer-service;1"].
                          getService(Ci.nsIObserverService);
    observerService.notifyObservers(stateString, "sessionstore-state-read", "");
    
    return stateString.data;
  },

  





  _readFile: function sss_readFile(aFile) {
    try {
      var stream = Cc["@mozilla.org/network/file-input-stream;1"].
                   createInstance(Ci.nsIFileInputStream);
      stream.init(aFile, 0x01, 0, 0);
      var cvstream = Cc["@mozilla.org/intl/converter-input-stream;1"].
                     createInstance(Ci.nsIConverterInputStream);
      cvstream.init(stream, "UTF-8", 1024, Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);
      
      var content = "";
      var data = {};
      while (cvstream.readString(4096, data)) {
        content += data.value;
      }
      cvstream.close();
      
      return content.replace(/\r\n?/g, "\n");
    }
    catch (ex) { } 
    
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


function NSGetModule(aCompMgr, aFileSpec) {
  return XPCOMUtils.generateModule([SessionStartup]);
}

