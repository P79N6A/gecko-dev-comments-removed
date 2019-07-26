














"use strict";




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "gSessionStartup",
                                   "@mozilla.org/browser/sessionstartup;1",
                                   "nsISessionStartup");

const kObservedTopics = [
  "sessionstore-windows-restored",
  "sessionstore-browser-state-restored",
  "download-manager-initialized",
  "download-manager-change-retention",
  "last-pb-context-exited",
  "browser-lastwindow-close-granted",
  "quit-application",
  "profile-change-teardown",
];




const kDownloadsUICid = Components.ID("{4d99321e-d156-455b-81f7-e7aa2308134f}");




const kDownloadsUIContractId = "@mozilla.org/download-manager-ui;1";




function DownloadsStartup() { }

DownloadsStartup.prototype = {
  classID: Components.ID("{49507fe5-2cee-4824-b6a3-e999150ce9b8}"),

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(DownloadsStartup),

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  
  

  observe: function DS_observe(aSubject, aTopic, aData)
  {
    switch (aTopic) {
      case "app-startup":
        kObservedTopics.forEach(
          function (topic) Services.obs.addObserver(this, topic, true),
          this);

        
        
        
        Components.manager.QueryInterface(Ci.nsIComponentRegistrar)
                          .registerFactory(kDownloadsUICid, "",
                                           kDownloadsUIContractId, null);
        break;

      case "sessionstore-windows-restored":
      case "sessionstore-browser-state-restored":
        
        
        
        
        if (gSessionStartup.sessionType != Ci.nsISessionStartup.NO_SESSION) {
          this._restoringSession = true;
        }
        this._ensureDataLoaded();
        break;

      case "download-manager-initialized":
        
        
        if (this._shuttingDown) {
          break;
        }

        
        
        
        DownloadsCommon.initializeAllDataLinks(
                        aSubject.QueryInterface(Ci.nsIDownloadManager));

        this._downloadsServiceInitialized = true;

        
        
        
        Services.tm.mainThread.dispatch(this._ensureDataLoaded.bind(this),
                                        Ci.nsIThread.DISPATCH_NORMAL);
        break;

      case "download-manager-change-retention":
        
        
        
        
        
        if (!DownloadsCommon.useToolkitUI) {
          let removeFinishedDownloads = Services.prefs.getBoolPref(
                            "browser.download.panel.removeFinishedDownloads");
          aSubject.QueryInterface(Ci.nsISupportsPRInt32)
                  .data = removeFinishedDownloads ? 0 : 2;
        }
        break;

      case "browser-lastwindow-close-granted":
        
        
        
        
        
        
        if (this._downloadsServiceInitialized &&
            !DownloadsCommon.useToolkitUI) {
          Services.downloads.cleanUp();
        }
        break;

      case "last-pb-context-exited":
        
        if (this._downloadsServiceInitialized &&
            !DownloadsCommon.useToolkitUI) {
          Services.downloads.cleanUpPrivate();
        }
        break;

      case "quit-application":
        
        
        
        
        
        this._shuttingDown = true;
        if (!this._downloadsServiceInitialized) {
          break;
        }

        DownloadsCommon.terminateAllDataLinks();

        
        
        if (!DownloadsCommon.useToolkitUI && aData != "restart") {
          this._cleanupOnShutdown = true;
        }
        break;

      case "profile-change-teardown":
        
        
        
        
        
        
        if (this._cleanupOnShutdown) {
          Services.downloads.cleanUp();
        }

        if (!DownloadsCommon.useToolkitUI) {
          
          
          
          
          this._firstSessionCompleted = true;
        }
        break;
    }
  },

  
  

  




  _restoringSession: false,

  





  _downloadsServiceInitialized: false,

  


  _shuttingDown: false,

  


  _cleanupOnShutdown: false,

  





  get _recoverAllDownloads() {
    return this._restoringSession ||
           (!DownloadsCommon.useToolkitUI && this._firstSessionCompleted);
  },

  


  get _firstSessionCompleted() {
    return Services.prefs
                   .getBoolPref("browser.download.panel.firstSessionCompleted");
  },

  set _firstSessionCompleted(aValue) {
    Services.prefs.setBoolPref("browser.download.panel.firstSessionCompleted",
                               aValue);
    return aValue;
  },

  


  _ensureDataLoaded: function DS_ensureDataLoaded()
  {
    if (!this._downloadsServiceInitialized) {
      return;
    }

    
    
    
    DownloadsCommon.ensureAllPersistentDataLoaded(!this._recoverAllDownloads);
  }
};




this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DownloadsStartup]);
