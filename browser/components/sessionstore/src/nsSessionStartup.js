
































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CID = Components.ID("{ec7a6c20-e081-11da-8ad9-0800200c9a66}");
const CONTRACT_ID = "@mozilla.org/browser/sessionstartup;1";
const CLASS_NAME = "Browser Session Startup Service";

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



  


  init: function sss_init() {
    this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefService).getBranch("browser.");

    
    if (!this._prefBranch.getBoolPref("sessionstore.enabled"))
      return;

    
    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);
    this._sessionFile = dirService.get("ProfD", Ci.nsILocalFile);
    this._sessionFile.append("sessionstore.js");
    
    
    var resumeFromCrash = this._prefBranch.getBoolPref("sessionstore.resume_from_crash");
    if (resumeFromCrash || this._doResumeSession()) {
      
      this._iniString = this._readFile(this._sessionFile);
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
    
    
    this._doRestore = this._lastSessionCrashed ? this._doRecoverSession() : this._doResumeSession();
    if (this._iniString && !this._doRestore) {
      this._iniString = null; 
    }
    if (this._prefBranch.getBoolPref("sessionstore.resume_session_once")) {
      this._prefBranch.setBoolPref("sessionstore.resume_session_once", false);
    }
    
    if (this.doRestore()) {
      
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
      break;
    case "final-ui-startup": 
      observerService.removeObserver(this, "final-ui-startup");
      this.init();
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
    return this._doRestore && this._iniString != null;
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
        
        var brandStringBundle = this._getStringBundle("chrome://branding/locale/brand.properties");
        var brandShortName = brandStringBundle.GetStringFromName("brandShortName");

        
        var ssStringBundle = this._getStringBundle("chrome://browser/locale/sessionstore.properties");
        var restoreTitle = ssStringBundle.formatStringFromName("restoredTitle", [brandShortName], 1);
        var restoreText = ssStringBundle.formatStringFromName("restoredMsg", [brandShortName], 1);
        var buttonTitle = ssStringBundle.GetStringFromName("buttonTitle");
        var cancelTitle = ssStringBundle.GetStringFromName("cancelTitle");

        var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                            getService(Ci.nsIPromptService);

        
        var flags = promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0 +
                    promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_1 +
                    promptService.BUTTON_POS_0_DEFAULT;
        
        var buttonChoice = promptService.confirmEx(null, restoreTitle, restoreText, 
                                          flags, buttonTitle, cancelTitle, null, 
                                          null, {});
        recover = (buttonChoice == 0);
      }
    }
    catch (ex) { dump(ex + "\n"); } 
    return recover;
  },

  




  _getStringBundle: function sss_getStringBundle(aURI) {
    var bundleService = Cc["@mozilla.org/intl/stringbundle;1"].
                        getService(Ci.nsIStringBundleService);
    var appLocale = Cc["@mozilla.org/intl/nslocaleservice;1"].
                    getService(Ci.nsILocaleService).getApplicationLocale();
    return bundleService.createBundle(aURI, appLocale);
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



  QueryInterface: function(aIID) {
    if (!aIID.equals(Ci.nsISupports) && !aIID.equals(Ci.nsIObserver) && 
      !aIID.equals(Ci.nsISupportsWeakReference) && 
      !aIID.equals(Ci.nsISessionStartup)) {
      Components.returnCode = Cr.NS_ERROR_NO_INTERFACE;
      return null;
    }
    
    return this;
  }
};





const SessionStartupModule = {

  getClassObject: function(aCompMgr, aCID, aIID) {
    if (aCID.equals(CID)) {
      return SessionStartupFactory;
    }
    
    Components.returnCode = Cr.NS_ERROR_NOT_REGISTERED;
    return null;
  },

  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType) {
    aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CID, CLASS_NAME, CONTRACT_ID, aFileSpec, aLocation, aType);

    var catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    catMan.addCategoryEntry("app-startup", CLASS_NAME, "service," + CONTRACT_ID, true, true);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType) {
    aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CID, aLocation);

    var catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    catMan.deleteCategoryEntry( "app-startup", "service," + CONTRACT_ID, true);
  },

  canUnload: function(aCompMgr) {
    return true;
  }
}



const SessionStartupFactory = {

  createInstance: function(aOuter, aIID) {
    if (aOuter != null) {
      Components.returnCode = Cr.NS_ERROR_NO_AGGREGATION;
      return null;
    }
    
    return (new SessionStartup()).QueryInterface(aIID);
  },

  lockFactory: function(aLock) { },

  QueryInterface: function(aIID) {
    if (!aIID.equals(Ci.nsISupports) && !aIID.equals(Ci.nsIModule) &&
        !aIID.equals(Ci.nsIFactory) && !aIID.equals(Ci.nsISessionStartup)) {
      Components.returnCode = Cr.NS_ERROR_NO_INTERFACE;
      return null;
    }
    
    return this;
  }
};

function NSGetModule(aComMgr, aFileSpec) {
  return SessionStartupModule;
}
