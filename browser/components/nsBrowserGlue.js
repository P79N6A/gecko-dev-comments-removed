




































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;



function BrowserGlue() {
  this._init();
  this._profileStarted = false;
}

BrowserGlue.prototype = {
  _saveSession: false,

  QueryInterface: function(iid) 
  {
     xpcomCheckInterfaces(iid, kServiceIIds, Cr.NS_ERROR_NO_INTERFACE);
     return this;
  }
,
  
  observe: function(subject, topic, data) 
  {
    switch(topic) {
      case "xpcom-shutdown":
        this._dispose();
        break;
      case "profile-before-change":
        this._onProfileChange();
        break;
      case "profile-change-teardown": 
        this._onProfileShutdown();
        break;
      case "final-ui-startup":
        this._onProfileStartup();
        break;
      case "browser:purge-session-history":
        
        const cs = Cc["@mozilla.org/consoleservice;1"].
                   getService(Ci.nsIConsoleService);
        cs.logStringMessage(null); 
        cs.reset();
        break;
      case "quit-application-requested":
        this._onQuitRequest(subject, data);
        break;
      case "quit-application-granted":
        if (this._saveSession) {
          var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                           getService(Ci.nsIPrefBranch);
          prefBranch.setBoolPref("browser.sessionstore.resume_session_once", true);
        }
        break;
    }
  }
, 
  
  _init: function() 
  {
    
    const osvr = Cc['@mozilla.org/observer-service;1'].
                 getService(Ci.nsIObserverService);
    osvr.addObserver(this, "profile-before-change", false);
    osvr.addObserver(this, "profile-change-teardown", false);
    osvr.addObserver(this, "xpcom-shutdown", false);
    osvr.addObserver(this, "final-ui-startup", false);
    osvr.addObserver(this, "browser:purge-session-history", false);
    osvr.addObserver(this, "quit-application-requested", false);
    osvr.addObserver(this, "quit-application-granted", false);
  },

  
  _dispose: function() 
  {
    
    const osvr = Cc['@mozilla.org/observer-service;1'].
                 getService(Ci.nsIObserverService);
    osvr.removeObserver(this, "profile-before-change");
    osvr.removeObserver(this, "profile-change-teardown");
    osvr.removeObserver(this, "xpcom-shutdown");
    osvr.removeObserver(this, "final-ui-startup");
    osvr.removeObserver(this, "browser:purge-session-history");
    osvr.removeObserver(this, "quit-application-requested");
    osvr.removeObserver(this, "quit-application-granted");
  },

  
  _onProfileStartup: function() 
  {
    
    try {
      var mustDisplayEULA = true;
      var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch);
      var EULAVersion = prefBranch.getIntPref("browser.EULA.version");
      mustDisplayEULA = !prefBranch.getBoolPref("browser.EULA." + EULAVersion + ".accepted");
    } catch(ex) {
    }

    if (mustDisplayEULA) {
      var ww2 = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                getService(Ci.nsIWindowWatcher);
      ww2.openWindow(null, "chrome://browser/content/EULA.xul", 
                     "_blank", "chrome,centerscreen,modal,resizable=yes", null);
    }

    this.Sanitizer.onStartup();
    
    var app = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo).
              QueryInterface(Ci.nsIXULRuntime);
    if (app.inSafeMode) {
      var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
               getService(Ci.nsIWindowWatcher);
      ww.openWindow(null, "chrome://browser/content/safeMode.xul", 
                    "_blank", "chrome,centerscreen,modal,resizable=no", null);
    }

    
    this._initPlaces();

    
    this._profileStarted = true;
  },

  _onProfileChange: function()
  {
    
    
    if (this._profileStarted) {
      
      this._shutdownPlaces();
    }
  },

  
  _onProfileShutdown: function() 
  {
    
    
    const appStartup = Cc['@mozilla.org/toolkit/app-startup;1'].
                       getService(Ci.nsIAppStartup);
    try {
      appStartup.enterLastWindowClosingSurvivalArea();

      this.Sanitizer.onShutdown();

    } catch(ex) {
    } finally {
      appStartup.exitLastWindowClosingSurvivalArea();
    }
  },

  _onQuitRequest: function(aCancelQuit, aQuitType)
  {
    var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);
    var windowcount = 0;
    var pagecount = 0;
    var browserEnum = wm.getEnumerator("navigator:browser");
    while (browserEnum.hasMoreElements()) {
      windowcount++;

      var browser = browserEnum.getNext();
      var tabbrowser = browser.document.getElementById("content");
      if (tabbrowser)
        pagecount += tabbrowser.browsers.length;
    }

    this._saveSession = false;
    if (pagecount < 2)
      return;

    if (aQuitType != "restart")
      aQuitType = "quit";

    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);
    var showPrompt = true;
    try {
      if (prefBranch.getIntPref("browser.startup.page") == 3 ||
          prefBranch.getBoolPref("browser.sessionstore.resume_session_once"))
        showPrompt = false;
      else
        showPrompt = prefBranch.getBoolPref("browser.warnOnQuit");
    } catch (ex) {}

    var buttonChoice = 0;
    if (showPrompt) {
      var bundleService = Cc["@mozilla.org/intl/stringbundle;1"].
                          getService(Ci.nsIStringBundleService);
      var quitBundle = bundleService.createBundle("chrome://browser/locale/quitDialog.properties");
      var brandBundle = bundleService.createBundle("chrome://branding/locale/brand.properties");

      var appName = brandBundle.GetStringFromName("brandShortName");
      var quitDialogTitle = quitBundle.formatStringFromName(aQuitType + "DialogTitle",
                                                              [appName], 1);
      var quitTitle = quitBundle.GetStringFromName(aQuitType + "Title");
      var cancelTitle = quitBundle.GetStringFromName("cancelTitle");
      var saveTitle = quitBundle.GetStringFromName("saveTitle");
      var neverAskText = quitBundle.GetStringFromName("neverAsk");

      var message;
      if (aQuitType == "restart")
        message = quitBundle.formatStringFromName("messageRestart",
                                                  [appName], 1);
      else if (windowcount == 1)
        message = quitBundle.formatStringFromName("messageNoWindows",
                                                  [appName], 1);
      else
        message = quitBundle.formatStringFromName("message",
                                                  [appName], 1);

      var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                          getService(Ci.nsIPromptService);

      var flags = promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0 +
                  promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_1 +
                  promptService.BUTTON_POS_0_DEFAULT;
      var neverAsk = {value:false};
      if (aQuitType != "restart")
        flags += promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_2;
      buttonChoice = promptService.confirmEx(null, quitDialogTitle, message,
                                   flags, quitTitle, cancelTitle, saveTitle,
                                   neverAskText, neverAsk);

      switch (buttonChoice) {
      case 0:
        if (neverAsk.value)
          prefBranch.setBoolPref("browser.warnOnQuit", false);
        break;
      case 1:
        aCancelQuit.QueryInterface(Ci.nsISupportsPRBool);
        aCancelQuit.data = true;
        break;
      case 2:
        
        
        if (neverAsk.value)
          prefBranch.setIntPref("browser.startup.page", 3);
        break;
      }

      this._saveSession = buttonChoice == 2;
    }
  },

  
  get Sanitizer() 
  {
    if(typeof(Sanitizer) != "function") { 
      Cc["@mozilla.org/moz/jssubscript-loader;1"].
      getService(Ci.mozIJSSubScriptLoader).
      loadSubScript("chrome://browser/content/sanitize.js", null);
    }
    return Sanitizer;
  },

  



  _initPlaces: function bg__initPlaces() {
    
    
    
    
    var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                  getService(Ci.nsINavHistoryService);

    var importBookmarks = false;
    try {
      var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch);
      importBookmarks = prefBranch.getBoolPref("browser.places.importBookmarksHTML");
    } catch(ex) {}

    if (!importBookmarks)
      return;

    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);

    var bookmarksFile = dirService.get("BMarks", Ci.nsILocalFile);

    if (bookmarksFile.exists()) {
      
      try {
        var importer = 
          Cc["@mozilla.org/browser/places/import-export-service;1"].
          getService(Ci.nsIPlacesImportExportService);
        importer.importHTMLFromFile(bookmarksFile, true);
      } catch(ex) {
      } finally {
        prefBranch.setBoolPref("browser.places.importBookmarksHTML", false);
      }

      
      
      var profDir = dirService.get("ProfD", Ci.nsILocalFile);
      var bookmarksBackup = profDir.clone();
      bookmarksBackup.append("bookmarks.preplaces.html");
      if (!bookmarksBackup.exists()) {
        
        try {
          bookmarksFile.copyTo(profDir, "bookmarks.preplaces.html");
        } catch(ex) {
          dump("nsBrowserGlue::_initPlaces(): copy of bookmarks.html to bookmarks.preplaces.html failed: " + ex + "\n");
        }
      }
    }
  },

  



  _shutdownPlaces: function bg__shutdownPlaces() {
    
    var importer =
      Cc["@mozilla.org/browser/places/import-export-service;1"].
      getService(Ci.nsIPlacesImportExportService);
    importer.backupBookmarksFile();
  },
  
  
  
  
  
  sanitize: function(aParentWindow) 
  {
    this.Sanitizer.sanitize(aParentWindow);
  }
}






const kServiceName = "Firefox Browser Glue Service";
const kServiceId = "{eab9012e-5f74-4cbc-b2b5-a590235513cc}";
const kServiceCtrId = "@mozilla.org/browser/browserglue;1";
const kServiceConstructor = BrowserGlue;

const kServiceCId = Components.ID(kServiceId);


const kServiceIIds = [ 
  Ci.nsIObserver,
  Ci.nsISupports,
  Ci.nsISupportsWeakReference,
  Ci.nsIBrowserGlue
  ];


const kServiceCats = ["app-startup"];


const kServiceFactory = {
  _instance: null,
  createInstance: function (outer, iid) 
  {
    if (outer != null) throw Cr.NS_ERROR_NO_AGGREGATION;

    xpcomCheckInterfaces(iid, kServiceIIds, 
                          Cr.NS_ERROR_INVALID_ARG);
    return this._instance == null ?
      this._instance = new kServiceConstructor() : this._instance;
  }
};

function xpcomCheckInterfaces(iid, iids, ex) {
  for (var j = iids.length; j-- >0;) {
    if (iid.equals(iids[j])) return true;
  }
  throw ex;
}



var Module = {
  registered: false,
  
  registerSelf: function(compMgr, fileSpec, location, type) 
  {
    if (!this.registered) {
      compMgr.QueryInterface(Ci.nsIComponentRegistrar)
             .registerFactoryLocation(kServiceCId,
                                      kServiceName,
                                      kServiceCtrId, 
                                      fileSpec,
                                      location, 
                                      type);
      const catman = Cc['@mozilla.org/categorymanager;1'].
                     getService(Ci.nsICategoryManager);
      var len = kServiceCats.length;
      for (var j = 0; j < len; j++) {
        catman.addCategoryEntry(kServiceCats[j],
          kServiceCtrId, kServiceCtrId, true, true);
      }
      this.registered = true;
    } 
  },
  
  unregisterSelf: function(compMgr, fileSpec, location) 
  {
    compMgr.QueryInterface(Ci.nsIComponentRegistrar)
           .unregisterFactoryLocation(kServiceCId, fileSpec);
    const catman = Cc['@mozilla.org/categorymanager;1'].
                   getService(Ci.nsICategoryManager);
    var len = kServiceCats.length;
    for (var j = 0; j < len; j++) {
      catman.deleteCategoryEntry(kServiceCats[j], kServiceCtrId, true);
    }
  },
  
  getClassObject: function(compMgr, cid, iid) 
  {
    if(cid.equals(kServiceCId))
      return kServiceFactory;
    
    throw Cr[
      iid.equals(Ci.nsIFactory)
      ? "NS_ERROR_NO_INTERFACE"
      : "NS_ERROR_NOT_IMPLEMENTED"
    ];
    
  },
  
  canUnload: function(compMgr) 
  {
    return true;
  }
};


function NSGetModule(compMgr, fileSpec) {
  return Module;
}
