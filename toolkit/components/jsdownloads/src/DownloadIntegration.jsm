










"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadIntegration",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadStore",
                                  "resource://gre/modules/DownloadStore.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadImport",
                                  "resource://gre/modules/DownloadImport.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadUIHelper",
                                  "resource://gre/modules/DownloadUIHelper.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gDownloadPlatform",
                                   "@mozilla.org/toolkit/download-platform;1",
                                   "mozIDownloadPlatform");
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





XPCOMUtils.defineLazyGetter(this, "gInternetZoneIdentifier", function() {
  return new TextEncoder().encode("[ZoneTransfer]\r\nZoneId=3\r\n");
});

const Timer = Components.Constructor("@mozilla.org/timer;1", "nsITimer",
                                     "initWithCallback");





















const kSaveDelayMs = 1500;





const kPrefImportedFromSqlite = "browser.download.importedFromSqlite";








this.DownloadIntegration = {
  
  _testMode: false,
  testPromptDownloads: 0,
  dontLoadList: false,
  dontLoadObservers: false,
  dontCheckParentalControls: false,
  shouldBlockInTest: false,
  dontOpenFileAndFolder: false,
  downloadDoneCalled: false,
  _deferTestOpenFile: null,
  _deferTestShowDir: null,
  _deferTestClearPrivateList: null,

  




  _store: null,

  


  get testMode() this._testMode,
  set testMode(mode) {
    this._downloadsDirectory = null;
    return (this._testMode = mode);
  },

  













  initializePublicDownloadList: function(aList) {
    return Task.spawn(function task_DI_initializePublicDownloadList() {
      if (this.dontLoadList) {
        
        
        new DownloadHistoryObserver(aList);
        return;
      }

      if (this._store) {
        throw new Error("initializePublicDownloadList may be called only once.");
      }

      this._store = new DownloadStore(aList, OS.Path.join(
                                                OS.Constants.Path.profileDir,
                                                "downloads.json"));
      this._store.onsaveitem = this.shouldPersistDownload.bind(this);

      if (this._importedFromSqlite) {
        try {
          yield this._store.load();
        } catch (ex) {
          Cu.reportError(ex);
        }
      } else {
        let sqliteDBpath = OS.Path.join(OS.Constants.Path.profileDir,
                                        "downloads.sqlite");

        if (yield OS.File.exists(sqliteDBpath)) {
          let sqliteImport = new DownloadImport(aList, sqliteDBpath);
          yield sqliteImport.import();

          let importCount = (yield aList.getAll()).length;
          if (importCount > 0) {
            try {
              yield this._store.save();
            } catch (ex) { }
          }

          
          OS.File.remove(sqliteDBpath).then(null, Cu.reportError);
        }

        Services.prefs.setBoolPref(kPrefImportedFromSqlite, true);

        
        
        OS.File.remove(OS.Path.join(OS.Constants.Path.profileDir,
                                    "downloads.rdf"));

      }

      
      
      
      
      
      
      
      yield new DownloadAutoSaveView(aList, this._store).initialize();
      new DownloadHistoryObserver(aList);
    }.bind(this));
  },

  
















  shouldPersistDownload: function (aDownload)
  {
    
    
    
    
    return aDownload.hasPartialData || !aDownload.stopped;
  },

  





  getSystemDownloadsDirectory: function DI_getSystemDownloadsDirectory() {
    return Task.spawn(function() {
      if (this._downloadsDirectory) {
        
        
        
        yield undefined;
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
#ifdef ANDROID
      
      
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

  









  downloadDone: function(aDownload) {
    return Task.spawn(function () {
#ifdef XP_WIN
      
      
      
      
      
      
      
      if (Services.prefs.getBoolPref("browser.download.saveZoneInformation")) {
        let file = new FileUtils.File(aDownload.target.path);
        if (file.isExecutable()) {
          try {
            let streamPath = aDownload.target.path + ":Zone.Identifier";
            let stream = yield OS.File.open(streamPath, { create: true });
            try {
              yield stream.write(gInternetZoneIdentifier);
            } finally {
              yield stream.close();
            }
          } catch (ex) {
            
            
            
            
            
            if (!(ex instanceof OS.File.Error) || ex.winLastError != 123) {
              Cu.reportError(ex);
            }
          }
        }
      }
#endif

      gDownloadPlatform.downloadDone(NetUtil.newURI(aDownload.source.url),
                                     new FileUtils.File(aDownload.target.path),
                                     aDownload.contentType,
                                     aDownload.source.isPrivate);
      this.downloadDoneCalled = true;
    }.bind(this));
  },

  


  _isImmersiveProcess: function() {
    
    return false;
  },

  



















  launchDownload: function (aDownload) {
    let deferred = Task.spawn(function DI_launchDownload_task() {
      let file = new FileUtils.File(aDownload.target.path);

#ifndef XP_WIN
      
      
      
      
      
      
      
      if (file.isExecutable() && !this.dontOpenFileAndFolder) {
        
        
        
        
        let shouldLaunch = yield DownloadUIHelper.getPrompter()
                                   .confirmLaunchExecutable(file.path);
        if (!shouldLaunch) {
          return;
        }
      }
#endif

      
      
      
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

    
    
    
    directory.append(DownloadUIHelper.strings.downloadsFolder);

    
    return OS.File.makeDir(directory.path, { ignoreExisting: true }).
             then(function() {
               return directory;
             });
  },

  





  _getDirectory: function DI_getDirectory(aName) {
    return Services.dirsvc.get(this.testMode ? "TmpD" : aName, Ci.nsIFile);
  },

  










  addListObservers: function DI_addListObservers(aList, aIsPrivate) {
    if (this.dontLoadObservers) {
      return Promise.resolve();
    }

    DownloadObserver.registerView(aList, aIsPrivate);
    if (!DownloadObserver.observersAdded) {
      DownloadObserver.observersAdded = true;
      Services.obs.addObserver(DownloadObserver, "quit-application-requested", true);
      Services.obs.addObserver(DownloadObserver, "offline-requested", true);
      Services.obs.addObserver(DownloadObserver, "last-pb-context-exiting", true);
      Services.obs.addObserver(DownloadObserver, "last-pb-context-exited", true);
    }
    return Promise.resolve();
  },

  





  get _importedFromSqlite() {
    try {
      return Services.prefs.getBoolPref(kPrefImportedFromSqlite);
    } catch (ex) {
      return false;
    }
  },
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

    
    aList.addView(downloadsView).then(null, Cu.reportError);
  },

  













  _confirmCancelDownloads: function DO_confirmCancelDownload(
    aCancel, aDownloadsCount, aPrompter, aPromptType) {
    
    if ((aCancel instanceof Ci.nsISupportsPRBool) && aCancel.data) {
      return;
    }
    
    if (DownloadIntegration.testMode) {
      DownloadIntegration.testPromptDownloads = aDownloadsCount;
      return;
    }

    aCancel.data = aPrompter.confirmCancelDownloads(aDownloadsCount, aPromptType);
  },

  
  

  observe: function DO_observe(aSubject, aTopic, aData) {
    let downloadsCount;
    let p = DownloadUIHelper.getPrompter();
    switch (aTopic) {
      case "quit-application-requested":
        downloadsCount = this._publicInProgressDownloads.size +
                         this._privateInProgressDownloads.size;
        this._confirmCancelDownloads(aSubject, downloadsCount, p, p.ON_QUIT);
        break;
      case "offline-requested":
        downloadsCount = this._publicInProgressDownloads.size +
                         this._privateInProgressDownloads.size;
        this._confirmCancelDownloads(aSubject, downloadsCount, p, p.ON_OFFLINE);
        break;
      case "last-pb-context-exiting":
        downloadsCount = this._privateInProgressDownloads.size;
        this._confirmCancelDownloads(aSubject, downloadsCount, p,
                                     p.ON_LEAVE_PRIVATE_BROWSING);
        break;
      case "last-pb-context-exited":
        let deferred = Task.spawn(function() {
          let list = yield Downloads.getList(Downloads.PRIVATE);
          let downloads = yield list.getAll();

          
          for (let download of downloads) {
            list.remove(download).then(null, Cu.reportError);
            download.finalize(true).then(null, Cu.reportError);
          }
        });
        
        if (DownloadIntegration.testMode) {
          deferred.then((value) => { DownloadIntegration._deferTestClearPrivateList.resolve("success"); },
                        (error) => { DownloadIntegration._deferTestClearPrivateList.reject(error); });
        }
        break;
    }
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};














function DownloadHistoryObserver(aList)
{
  this._list = aList;
  PlacesUtils.history.addObserver(this, false);
}

DownloadHistoryObserver.prototype = {
  


  _list: null,

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver]),

  
  

  onDeleteURI: function DL_onDeleteURI(aURI, aGUID) {
    this._list.removeFinished(download => aURI.equals(NetUtil.newURI(
                                                      download.source.url)));
  },

  onClearHistory: function DL_onClearHistory() {
    this._list.removeFinished();
  },

  onTitleChanged: function () {},
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onVisit: function () {},
  onPageChanged: function () {},
  onDeleteVisits: function () {},
};


















function DownloadAutoSaveView(aList, aStore) {
  this._list = aList;
  this._store = aStore;
  this._downloadsMap = new Map();
}

DownloadAutoSaveView.prototype = {
  


  _list: null,

  


  _store: null,

  


  _initialized: false,

  






  initialize: function ()
  {
    
    
    return this._list.addView(this).then(() => this._initialized = true);
  },

  




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
