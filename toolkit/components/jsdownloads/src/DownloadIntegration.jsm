










"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadIntegration",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DownloadStore",
                                  "resource://gre/modules/DownloadStore.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "gEnvironment",
                                   "@mozilla.org/process/environment;1",
                                   "nsIEnvironment");
XPCOMUtils.defineLazyServiceGetter(this, "gMIMEService",
                                   "@mozilla.org/mime;1",
                                   "nsIMIMEService");
XPCOMUtils.defineLazyServiceGetter(this, "gExternalProtocolService",
                                   "@mozilla.org/uriloader/external-protocol-service;1",
                                   "nsIExternalProtocolService");

XPCOMUtils.defineLazyGetter(this, "gParentalControlsService", function() {
  if ("@mozilla.org/parental-controls-service;1" in Cc) {
    return Cc["@mozilla.org/parental-controls-service;1"]
      .createInstance(Ci.nsIParentalControlsService);
  }
  return null;
});

XPCOMUtils.defineLazyGetter(this, "gStringBundle", function() {
  return Services.strings.
    createBundle("chrome://mozapps/locale/downloads/downloads.properties");
});

const Timer = Components.Constructor("@mozilla.org/timer;1", "nsITimer",
                                     "initWithCallback");





















const kSaveDelayMs = 1500;








this.DownloadIntegration = {
  
  _testMode: false,
  dontLoad: false,
  dontCheckParentalControls: false,
  shouldBlockInTest: false,
  dontOpenFileAndFolder: false,
  _deferTestOpenFile: null,
  _deferTestShowDir: null,

  




  _store: null,

  


  get testMode() this._testMode,
  set testMode(mode) {
    this._downloadsDirectory = null;
    return (this._testMode = mode);
  },

  













  loadPersistent: function DI_loadPersistent(aList)
  {
    if (this.dontLoad) {
      return Promise.resolve();
    }

    if (this._store) {
      throw new Error("loadPersistent may be called only once.");
    }

    this._store = new DownloadStore(aList, OS.Path.join(
                                              OS.Constants.Path.profileDir,
                                              "downloads.json"));
    this._store.onsaveitem = this.shouldPersistDownload.bind(this);

    
    
    return this._store.load().then(null, Cu.reportError).then(() => {
      new DownloadAutoSaveView(aList, this._store);
    });
  },

  
















  shouldPersistDownload: function (aDownload)
  {
    
    
    
    
    return aDownload.hasPartialData || !aDownload.stopped;
  },

  





  getSystemDownloadsDirectory: function DI_getSystemDownloadsDirectory() {
    return Task.spawn(function() {
      if (this._downloadsDirectory) {
        
        
        
        yield;
        throw new Task.Result(this._downloadsDirectory);
      }

      let directory = null;
#ifdef XP_MACOSX
      directory = this._getDirectory("DfltDwnld");
#elifdef XP_WIN
      
      
      let version = parseFloat(Services.sysinfo.getProperty("version"));
      if (version < 6) {
        directory = yield this._createDownloadsDirectory("Pers");
      } else {
        directory = this._getDirectory("DfltDwnld");
      }
#elifdef XP_UNIX
#ifdef MOZ_PLATFORM_MAEMO
      
      
      
      
      
      directory = this._getDirectory("XDGDocs");
#elifdef ANDROID
      
      
      let directoryPath = gEnvironment.get("DOWNLOADS_DIRECTORY");
      if (!directoryPath) {
        throw new Components.Exception("DOWNLOADS_DIRECTORY is not set.",
                                       Cr.NS_ERROR_FILE_UNRECOGNIZED_PATH);
      }
      directory = new FileUtils.File(directoryPath);
#else
      
      
      try {
        directory = this._getDirectory("DfltDwnld");
      } catch(e) {
        directory = yield this._createDownloadsDirectory("Home");
      }
#endif
#else
      directory = yield this._createDownloadsDirectory("Home");
#endif
      this._downloadsDirectory = directory;
      throw new Task.Result(this._downloadsDirectory);
    }.bind(this));
  },
  _downloadsDirectory: null,

  





  getUserDownloadsDirectory: function DI_getUserDownloadsDirectory() {
    return Task.spawn(function() {
      let directory = null;
      let prefValue = 1;

      try {
        prefValue = Services.prefs.getIntPref("browser.download.folderList");
      } catch(e) {}

      switch(prefValue) {
        case 0: 
          directory = this._getDirectory("Desk");
          break;
        case 1: 
          directory = yield this.getSystemDownloadsDirectory();
          break;
        case 2: 
          try {
            directory = Services.prefs.getComplexValue("browser.download.dir",
                                                       Ci.nsIFile);
            yield OS.File.makeDir(directory.path, { ignoreExisting: true });
          } catch(ex) {
            
            directory = yield this.getSystemDownloadsDirectory();
          }
          break;
        default:
          directory = yield this.getSystemDownloadsDirectory();
      }
      throw new Task.Result(directory);
    }.bind(this));
  },

  





  getTemporaryDownloadsDirectory: function DI_getTemporaryDownloadsDirectory() {
    return Task.spawn(function() {
      let directory = null;
#ifdef XP_MACOSX
      directory = yield this.getUserDownloadsDirectory();
#elifdef ANDROID
      directory = yield this.getSystemDownloadsDirectory();
#else
      
      
      if (this._isImmersiveProcess()) {
        directory = yield this.getSystemDownloadsDirectory();
      } else {
        directory = this._getDirectory("TmpD");
      }
#endif
      throw new Task.Result(directory);
    }.bind(this));
  },

  








  shouldBlockForParentalControls: function DI_shouldBlockForParentalControls(aDownload) {
    if (this.dontCheckParentalControls) {
      return Promise.resolve(this.shouldBlockInTest);
    }

    let isEnabled = gParentalControlsService &&
                    gParentalControlsService.parentalControlsEnabled;
    let shouldBlock = isEnabled &&
                      gParentalControlsService.blockFileDownloadsEnabled;

    
    if (isEnabled && gParentalControlsService.loggingEnabled) {
      gParentalControlsService.log(gParentalControlsService.ePCLog_FileDownload,
                                   shouldBlock,
                                   NetUtil.newURI(aDownload.source.url), null);
    }

    return Promise.resolve(shouldBlock);
  },

  


  _isImmersiveProcess: function() {
    
    return false;
  },

  



















  launchDownload: function (aDownload) {
    let deferred = Task.spawn(function DI_launchDownload_task() {
      let file = new FileUtils.File(aDownload.target.path);

      
      
      
      let fileExtension = null, mimeInfo = null;
      let match = file.leafName.match(/\.([^.]+)$/);
      if (match) {
        fileExtension = match[1];
      }

      try {
        
        
        
        mimeInfo = gMIMEService.getFromTypeAndExtension(aDownload.contentType,
                                                        fileExtension);
      } catch (e) { }

      if (aDownload.launcherPath) {
        if (!mimeInfo) {
          
          
          
          throw new Error(
            "Unable to create nsIMIMEInfo to launch a custom application");
        }

        
        let localHandlerApp = Cc["@mozilla.org/uriloader/local-handler-app;1"]
                                .createInstance(Ci.nsILocalHandlerApp);
        localHandlerApp.executable = new FileUtils.File(aDownload.launcherPath);

        mimeInfo.preferredApplicationHandler = localHandlerApp;
        mimeInfo.preferredAction = Ci.nsIMIMEInfo.useHelperApp;

        
        if (this.dontOpenFileAndFolder) {
          throw new Task.Result(mimeInfo);
        }

        mimeInfo.launchWithFile(file);
        return;
      }

      
      
      if (this.dontOpenFileAndFolder) {
        throw new Task.Result(null);
      }

      
      
      if (mimeInfo) {
        mimeInfo.preferredAction = Ci.nsIMIMEInfo.useSystemDefault;

        try {
          mimeInfo.launchWithFile(file);
          return;
        } catch (ex) { }
      }

      
      
      try {
        file.launch();
        return;
      } catch (ex) { }

      
      
      gExternalProtocolService.loadUrl(NetUtil.newURI(file));
      yield undefined;
    }.bind(this));

    if (this.dontOpenFileAndFolder) {
      deferred.then((value) => { this._deferTestOpenFile.resolve(value); },
                    (error) => { this._deferTestOpenFile.reject(error); });
    }

    return deferred;
  },

  













  showContainingDirectory: function (aFilePath) {
    let deferred = Task.spawn(function DI_showContainingDirectory_task() {
      let file = new FileUtils.File(aFilePath);

      if (this.dontOpenFileAndFolder) {
        return;
      }

      try {
        
        file.reveal();
        return;
      } catch (ex) { }

      
      
      let parent = file.parent;
      if (!parent) {
        throw new Error(
          "Unexpected reference to a top-level directory instead of a file");
      }

      try {
        
        parent.launch();
        return;
      } catch (ex) { }

      
      
      gExternalProtocolService.loadUrl(NetUtil.newURI(parent));
      yield undefined;
    }.bind(this));

    if (this.dontOpenFileAndFolder) {
      deferred.then((value) => { this._deferTestShowDir.resolve("success"); },
                    (error) => { this._deferTestShowDir.reject(error); });
    }

    return deferred;
  },

  






  _createDownloadsDirectory: function DI_createDownloadsDirectory(aName) {
    let directory = this._getDirectory(aName);
    directory.append(gStringBundle.GetStringFromName("downloadsFolder"));

    
    return OS.File.makeDir(directory.path, { ignoreExisting: true }).
             then(function() {
               return directory;
             });
  },

  





  _getDirectory: function DI_getDirectory(aName) {
    return Services.dirsvc.get(this.testMode ? "TmpD" : aName, Ci.nsIFile);
  },

  










  addListObservers: function DI_addListObservers(aList, aIsPrivate) {
    if (this.dontLoad) {
      return Promise.resolve();
    }

    DownloadObserver.registerView(aList, aIsPrivate);
    if (!DownloadObserver.observersAdded) {
      DownloadObserver.observersAdded = true;
      Services.obs.addObserver(DownloadObserver, "quit-application-requested", true);
      Services.obs.addObserver(DownloadObserver, "offline-requested", true);
      Services.obs.addObserver(DownloadObserver, "last-pb-context-exiting", true);
    }
    return Promise.resolve();
  }
};




this.DownloadObserver = {
  


  observersAdded: false,

  




  _publicInProgressDownloads: new Set(),

  




  _privateInProgressDownloads: new Set(),

  









  registerView: function DO_registerView(aList, aIsPrivate) {
    let downloadsSet = aIsPrivate ? this._privateInProgressDownloads
                                  : this._publicInProgressDownloads;
    let downloadsView = {
      onDownloadAdded: function DO_V_onDownloadAdded(aDownload) {
        if (!aDownload.stopped) {
          downloadsSet.add(aDownload);
        }
      },
      onDownloadChanged: function DO_V_onDownloadChanged(aDownload) {
        if (aDownload.stopped) {
          downloadsSet.delete(aDownload);
        } else {
          downloadsSet.add(aDownload);
        }
      },
      onDownloadRemoved: function DO_V_onDownloadRemoved(aDownload) {
        downloadsSet.delete(aDownload);
      }
    };

    aList.addView(downloadsView);
  },

  















  _confirmCancelDownloads: function DO_confirmCancelDownload(
    aCancel, aDownloadsCount, aIdTitle, aIdMessageSingle, aIdMessageMultiple, aIdButton) {
    
    if ((aCancel instanceof Ci.nsISupportsPRBool) && aCancel.data) {
      return;
    }
    
    if (aDownloadsCount <= 0) {
      return;
    }

    let win = Services.wm.getMostRecentWindow("navigator:browser");
    let buttonFlags = (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_0) +
                      (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_1);
    let title = gStringBundle.GetStringFromName(aIdTitle);
    let dontQuitButton = gStringBundle.GetStringFromName(aIdButton);
    let quitButton;
    let message;

    if (aDownloadsCount > 1) {
      message = gStringBundle.formatStringFromName(aIdMessageMultiple,
                                                   [aDownloadsCount], 1);
      quitButton = gStringBundle.formatStringFromName("cancelDownloadsOKTextMultiple",
                                                      [aDownloadsCount], 1);
    } else {
      message = gStringBundle.GetStringFromName(aIdMessageSingle);
      quitButton = gStringBundle.GetStringFromName("cancelDownloadsOKText");
    }

    let rv = Services.prompt.confirmEx(win, title, message, buttonFlags,
                                       quitButton, dontQuitButton, null, null, {});
    aCancel.data = (rv == 1);
  },

  
  

  observe: function DO_observe(aSubject, aTopic, aData) {
    let downloadsCount;
    switch (aTopic) {
      case "quit-application-requested":
        downloadsCount = this._publicInProgressDownloads.size +
                         this._privateInProgressDownloads.size;
#ifndef XP_MACOSX
        this._confirmCancelDownloads(aSubject, downloadsCount,
                                     "quitCancelDownloadsAlertTitle",
                                     "quitCancelDownloadsAlertMsg",
                                     "quitCancelDownloadsAlertMsgMultiple",
                                     "dontQuitButtonWin");
#else
        this._confirmCancelDownloads(aSubject, downloadsCount,
                                     "quitCancelDownloadsAlertTitle",
                                     "quitCancelDownloadsAlertMsgMac",
                                     "quitCancelDownloadsAlertMsgMacMultiple",
                                     "dontQuitButtonMac");
#endif
        break;
      case "offline-requested":
        downloadsCount = this._publicInProgressDownloads.size +
                         this._privateInProgressDownloads.size;
        this._confirmCancelDownloads(aSubject, downloadsCount,
                                     "offlineCancelDownloadsAlertTitle",
                                     "offlineCancelDownloadsAlertMsg",
                                     "offlineCancelDownloadsAlertMsgMultiple",
                                     "dontGoOfflineButton");
        break;
      case "last-pb-context-exiting":
        this._confirmCancelDownloads(aSubject,
                                     this._privateInProgressDownloads.size,
                                     "leavePrivateBrowsingCancelDownloadsAlertTitle",
                                     "leavePrivateBrowsingWindowsCancelDownloadsAlertMsg",
                                     "leavePrivateBrowsingWindowsCancelDownloadsAlertMsgMultiple",
                                     "dontLeavePrivateBrowsingButton");
        break;
    }
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};











function DownloadAutoSaveView(aList, aStore) {
  this._store = aStore;
  this._downloadsMap = new Map();

  
  
  aList.addView(this);
  this._initialized = true;
}

DownloadAutoSaveView.prototype = {
  


  _initialized: false,

  


  _store: null,

  




  _downloadsMap: null,

  




  _shouldSave: false,

  







  _timer: null,

  


  _save: function ()
  {
    Task.spawn(function () {
      
      this._shouldSave = false;

      
      try {
        yield this._store.save();
      } catch (ex) {
        Cu.reportError(ex);
      }

      
      this._timer = null;
      if (this._shouldSave) {
        this.saveSoon();
      }
    }.bind(this)).then(null, Cu.reportError);
  },

  



  saveSoon: function ()
  {
    this._shouldSave = true;
    if (!this._timer) {
      this._timer = new Timer(this._save.bind(this), kSaveDelayMs,
                              Ci.nsITimer.TYPE_ONE_SHOT);
    }
  },

  
  

  onDownloadAdded: function (aDownload)
  {
    if (DownloadIntegration.shouldPersistDownload(aDownload)) {
      this._downloadsMap.set(aDownload, aDownload.getSerializationHash());
      if (this._initialized) {
        this.saveSoon();
      }
    }
  },

  onDownloadChanged: function (aDownload)
  {
    if (!DownloadIntegration.shouldPersistDownload(aDownload)) {
      if (this._downloadsMap.has(aDownload)) {
        this._downloadsMap.delete(aDownload);
        this.saveSoon();
      }
      return;
    }

    let hash = aDownload.getSerializationHash();
    if (this._downloadsMap.get(aDownload) != hash) {
      this._downloadsMap.set(aDownload, hash);
      this.saveSoon();
    }
  },

  onDownloadRemoved: function (aDownload)
  {
    if (this._downloadsMap.has(aDownload)) {
      this._downloadsMap.delete(aDownload);
      this.saveSoon();
    }
  },
};
