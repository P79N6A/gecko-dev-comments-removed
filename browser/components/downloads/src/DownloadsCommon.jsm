





"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadsCommon",
];






























const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadUtils",
                                  "resource://gre/modules/DownloadUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
                                  "resource:///modules/RecentWindow.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadsLogger",
                                  "resource:///modules/DownloadsLogger.jsm");

const nsIDM = Ci.nsIDownloadManager;

const kDownloadsStringBundleUrl =
  "chrome://browser/locale/downloads/downloads.properties";

const kPrefBdmScanWhenDone =   "browser.download.manager.scanWhenDone";
const kPrefBdmAlertOnExeOpen = "browser.download.manager.alertOnEXEOpen";

const kDownloadsStringsRequiringFormatting = {
  sizeWithUnits: true,
  shortTimeLeftSeconds: true,
  shortTimeLeftMinutes: true,
  shortTimeLeftHours: true,
  shortTimeLeftDays: true,
  statusSeparator: true,
  statusSeparatorBeforeNumber: true,
  fileExecutableSecurityWarning: true
};

const kDownloadsStringsRequiringPluralForm = {
  otherDownloads2: true
};

XPCOMUtils.defineLazyGetter(this, "DownloadsLocalFileCtor", function () {
  return Components.Constructor("@mozilla.org/file/local;1",
                                "nsILocalFile", "initWithPath");
});

const kPartialDownloadSuffix = ".part";

const kPrefDebug = "browser.download.debug";

let DebugPrefObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),
  observe: function PDO_observe(aSubject, aTopic, aData) {
    this.debugEnabled = Services.prefs.getBoolPref(kPrefDebug);
  }
}

XPCOMUtils.defineLazyGetter(DebugPrefObserver, "debugEnabled", function () {
  Services.prefs.addObserver(kPrefDebug, DebugPrefObserver, true);
  return Services.prefs.getBoolPref(kPrefDebug);
});









this.DownloadsCommon = {
  log: function DC_log(...aMessageArgs) {
    delete this.log;
    this.log = function DC_log(...aMessageArgs) {
      if (!DebugPrefObserver.debugEnabled) {
        return;
      }
      DownloadsLogger.log.apply(DownloadsLogger, aMessageArgs);
    }
    this.log.apply(this, aMessageArgs);
  },

  error: function DC_error(...aMessageArgs) {
    delete this.error;
    this.error = function DC_error(...aMessageArgs) {
      if (!DebugPrefObserver.debugEnabled) {
        return;
      }
      DownloadsLogger.reportError.apply(DownloadsLogger, aMessageArgs);
    }
    this.error.apply(this, aMessageArgs);
  },
  




  get strings()
  {
    let strings = {};
    let sb = Services.strings.createBundle(kDownloadsStringBundleUrl);
    let enumerator = sb.getSimpleEnumeration();
    while (enumerator.hasMoreElements()) {
      let string = enumerator.getNext().QueryInterface(Ci.nsIPropertyElement);
      let stringName = string.key;
      if (stringName in kDownloadsStringsRequiringFormatting) {
        strings[stringName] = function () {
          
          return sb.formatStringFromName(stringName,
                                         Array.slice(arguments, 0),
                                         arguments.length);
        };
      } else if (stringName in kDownloadsStringsRequiringPluralForm) {
        strings[stringName] = function (aCount) {
          
          let formattedString = sb.formatStringFromName(stringName,
                                         Array.slice(arguments, 0),
                                         arguments.length);
          return PluralForm.get(aCount, formattedString);
        };
      } else {
        strings[stringName] = string.value;
      }
    }
    delete this.strings;
    return this.strings = strings;
  },

  









  formatTimeLeft: function DC_formatTimeLeft(aSeconds)
  {
    
    let seconds = Math.round(aSeconds);
    if (!seconds) {
      return "";
    } else if (seconds <= 30) {
      return DownloadsCommon.strings["shortTimeLeftSeconds"](seconds);
    }
    let minutes = Math.round(aSeconds / 60);
    if (minutes < 60) {
      return DownloadsCommon.strings["shortTimeLeftMinutes"](minutes);
    }
    let hours = Math.round(minutes / 60);
    if (hours < 48) { 
      return DownloadsCommon.strings["shortTimeLeftHours"](hours);
    }
    let days = Math.round(hours / 24);
    return DownloadsCommon.strings["shortTimeLeftDays"](Math.min(days, 99));
  },

  




  get useToolkitUI()
  {
    try {
      return Services.prefs.getBoolPref("browser.download.useToolkitUI");
    } catch (ex) { }
    return false;
  },

  






  getData: function DC_getData(aWindow) {
    if (PrivateBrowsingUtils.isWindowPrivate(aWindow)) {
      return PrivateDownloadsData;
    } else {
      return DownloadsData;
    }
  },

  









  initializeAllDataLinks: function DC_initializeAllDataLinks(aDownloadManagerService) {
    DownloadsData.initializeDataLink(aDownloadManagerService);
    PrivateDownloadsData.initializeDataLink(aDownloadManagerService);
  },

  



  terminateAllDataLinks: function DC_terminateAllDataLinks() {
    DownloadsData.terminateDataLink();
    PrivateDownloadsData.terminateDataLink();
  },

  






  ensureAllPersistentDataLoaded:
  function DC_ensureAllPersistentDataLoaded(aActiveOnly) {
    DownloadsData.ensurePersistentDataLoaded(aActiveOnly);
  },

  




  getIndicatorData: function DC_getIndicatorData(aWindow) {
    if (PrivateBrowsingUtils.isWindowPrivate(aWindow)) {
      return PrivateDownloadsIndicatorData;
    } else {
      return DownloadsIndicatorData;
    }
  },

  









  getSummary: function DC_getSummary(aWindow, aNumToExclude)
  {
    if (PrivateBrowsingUtils.isWindowPrivate(aWindow)) {
      if (this._privateSummary) {
        return this._privateSummary;
      }
      return this._privateSummary = new DownloadsSummaryData(true, aNumToExclude);
    } else {
      if (this._summary) {
        return this._summary;
      }
      return this._summary = new DownloadsSummaryData(false, aNumToExclude);
    }
  },
  _summary: null,
  _privateSummary: null,

  




















  summarizeDownloads: function DC_summarizeDownloads(aDataItems)
  {
    let summary = {
      numActive: 0,
      numPaused: 0,
      numScanning: 0,
      numDownloading: 0,
      totalSize: 0,
      totalTransferred: 0,
      
      
      
      
      slowestSpeed: Infinity,
      rawTimeLeft: -1,
      percentComplete: -1
    }

    for (let dataItem of aDataItems) {
      summary.numActive++;
      switch (dataItem.state) {
        case nsIDM.DOWNLOAD_PAUSED:
          summary.numPaused++;
          break;
        case nsIDM.DOWNLOAD_SCANNING:
          summary.numScanning++;
          break;
        case nsIDM.DOWNLOAD_DOWNLOADING:
          summary.numDownloading++;
          if (dataItem.maxBytes > 0 && dataItem.speed > 0) {
            let sizeLeft = dataItem.maxBytes - dataItem.currBytes;
            summary.rawTimeLeft = Math.max(summary.rawTimeLeft,
                                           sizeLeft / dataItem.speed);
            summary.slowestSpeed = Math.min(summary.slowestSpeed,
                                            dataItem.speed);
          }
          break;
      }
      
      if (dataItem.maxBytes > 0 &&
          dataItem.state != nsIDM.DOWNLOAD_CANCELED &&
          dataItem.state != nsIDM.DOWNLOAD_FAILED) {
        summary.totalSize += dataItem.maxBytes;
        summary.totalTransferred += dataItem.currBytes;
      }
    }

    if (summary.numActive != 0 && summary.totalSize != 0 &&
        summary.numActive != summary.numScanning) {
      summary.percentComplete = (summary.totalTransferred /
                                 summary.totalSize) * 100;
    }

    if (summary.slowestSpeed == Infinity) {
      summary.slowestSpeed = 0;
    }

    return summary;
  },

  








  smoothSeconds: function DC_smoothSeconds(aSeconds, aLastSeconds)
  {
    
    
    
    let shouldApplySmoothing = aLastSeconds >= 0 &&
                               aSeconds > aLastSeconds / 2;
    if (shouldApplySmoothing) {
      
      
      let (diff = aSeconds - aLastSeconds) {
        aSeconds = aLastSeconds + (diff < 0 ? .3 : .1) * diff;
      }

      
      
      let diff = aSeconds - aLastSeconds;
      let diffPercent = diff / aLastSeconds * 100;
      if (Math.abs(diff) < 5 || Math.abs(diffPercent) < 5) {
        aSeconds = aLastSeconds - (diff < 0 ? .4 : .2);
      }
    }

    
    
    
    return aLastSeconds = Math.max(aSeconds, 1);
  },

  









  openDownloadedFile: function DC_openDownloadedFile(aFile, aMimeInfo, aOwnerWindow) {
    if (!(aFile instanceof Ci.nsIFile))
      throw new Error("aFile must be a nsIFile object");
    if (aMimeInfo && !(aMimeInfo instanceof Ci.nsIMIMEInfo))
      throw new Error("Invalid value passed for aMimeInfo");
    if (!(aOwnerWindow instanceof Ci.nsIDOMWindow))
      throw new Error("aOwnerWindow must be a dom-window object");

    
    if (aFile.isExecutable()) {
      let showAlert = true;
      try {
        showAlert = Services.prefs.getBoolPref(kPrefBdmAlertOnExeOpen);
      } catch (ex) { }

      
      
      if (DownloadsCommon.isWinVistaOrHigher) {
        try {
          if (Services.prefs.getBoolPref(kPrefBdmScanWhenDone)) {
            showAlert = false;
          }
        } catch (ex) { }
      }

      if (showAlert) {
        let name = aFile.leafName;
        let message =
          DownloadsCommon.strings.fileExecutableSecurityWarning(name, name);
        let title =
          DownloadsCommon.strings.fileExecutableSecurityWarningTitle;
        let dontAsk =
          DownloadsCommon.strings.fileExecutableSecurityWarningDontAsk;

        let checkbox = { value: false };
        let open = Services.prompt.confirmCheck(aOwnerWindow, title, message,
                                                dontAsk, checkbox);
        if (!open) {
          return;
        }

        Services.prefs.setBoolPref(kPrefBdmAlertOnExeOpen,
                                   !checkbox.value);
      }
    }

    
    try {
      if (aMimeInfo && aMimeInfo.preferredAction == aMimeInfo.useHelperApp) {
        aMimeInfo.launchWithFile(aFile);
        return;
      }
    }
    catch(ex) { }

    
    
    try {
      aFile.launch();
    }
    catch(ex) {
      
      
      Cc["@mozilla.org/uriloader/external-protocol-service;1"]
        .getService(Ci.nsIExternalProtocolService)
        .loadUrl(NetUtil.newURI(aFile));
    }
  },

  






  showDownloadedFile: function DC_showDownloadedFile(aFile) {
    if (!(aFile instanceof Ci.nsIFile))
      throw new Error("aFile must be a nsIFile object");
    try {
      
      aFile.reveal();
    } catch (ex) {
      
      
      let parent = aFile.parent;
      if (parent) {
        try {
          
          parent.launch();
        } catch (ex) {
          
          
          Cc["@mozilla.org/uriloader/external-protocol-service;1"]
            .getService(Ci.nsIExternalProtocolService)
            .loadUrl(NetUtil.newURI(parent));
        }
      }
    }
  }
};




XPCOMUtils.defineLazyGetter(DownloadsCommon, "isWinVistaOrHigher", function () {
  let os = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
  if (os != "WINNT") {
    return false;
  }
  let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
  return parseFloat(sysInfo.getProperty("version")) >= 6;
});























function DownloadsDataCtor(aPrivate) {
  this._isPrivate = aPrivate;

  
  
  
  
  
  this.dataItems = {};

  
  
  this._views = [];
}

DownloadsDataCtor.prototype = {
  








  initializeDataLink: function DD_initializeDataLink(aDownloadManagerService)
  {
    
    aDownloadManagerService.addPrivacyAwareListener(this);
    Services.obs.addObserver(this, "download-manager-remove-download-guid", false);
  },

  


  terminateDataLink: function DD_terminateDataLink()
  {
    this._terminateDataAccess();

    
    Services.obs.removeObserver(this, "download-manager-remove-download-guid");
    Services.downloads.removeListener(this);
  },

  
  

  







  addView: function DD_addView(aView)
  {
    this._views.push(aView);
    this._updateView(aView);
  },

  





  removeView: function DD_removeView(aView)
  {
    let index = this._views.indexOf(aView);
    if (index != -1) {
      this._views.splice(index, 1);
    }
  },

  





  _updateView: function DD_updateView(aView)
  {
    
    aView.onDataLoadStarting();

    
    
    let loadedItemsArray = [dataItem
                            for each (dataItem in this.dataItems)
                            if (dataItem)];
    loadedItemsArray.sort(function(a, b) b.startTime - a.startTime);
    loadedItemsArray.forEach(
      function (dataItem) aView.onDataItemAdded(dataItem, false)
    );

    
    if (!this._pendingStatement) {
      aView.onDataLoadCompleted();
    }
  },

  
  

  


  clear: function DD_clear()
  {
    this._terminateDataAccess();
    this.dataItems = {};
  },

  


























  _getOrAddDataItem: function DD_getOrAddDataItem(aSource, aMayReuseGUID)
  {
    let downloadGuid = (aSource instanceof Ci.nsIDownload)
                       ? aSource.guid
                       : aSource.getResultByName("guid");
    if (downloadGuid in this.dataItems) {
      let existingItem = this.dataItems[downloadGuid];
      if (existingItem || !aMayReuseGUID) {
        
        return existingItem;
      }
    }
    DownloadsCommon.log("Creating a new DownloadsDataItem with downloadGuid =",
                        downloadGuid);
    let dataItem = new DownloadsDataItem(aSource);
    this.dataItems[downloadGuid] = dataItem;

    
    let addToStartOfList = aSource instanceof Ci.nsIDownload;
    this._views.forEach(
      function (view) view.onDataItemAdded(dataItem, addToStartOfList)
    );
    return dataItem;
  },

  




  _removeDataItem: function DD_removeDataItem(aDownloadId)
  {
    if (aDownloadId in this.dataItems) {
      let dataItem = this.dataItems[aDownloadId];
      this.dataItems[aDownloadId] = null;
      this._views.forEach(
        function (view) view.onDataItemRemoved(dataItem)
      );
    }
  },

  
  

  


  _pendingStatement: null,

  




  _loadState: 0,

  
  get kLoadNone() 0,
  
  get kLoadActive() 1,
  
  get kLoadAll() 2,

  






  ensurePersistentDataLoaded:
  function DD_ensurePersistentDataLoaded(aActiveOnly)
  {
    if (this == PrivateDownloadsData) {
      Cu.reportError("ensurePersistentDataLoaded should not be called on PrivateDownloadsData");
      return;
    }

    if (this._pendingStatement) {
      
      return;
    }

    if (aActiveOnly) {
      if (this._loadState == this.kLoadNone) {
        DownloadsCommon.log("Loading only active downloads from the persistence database");
        
        this._views.forEach(
          function (view) view.onDataLoadStarting()
        );

        
        
        let downloads = Services.downloads.activeDownloads;
        while (downloads.hasMoreElements()) {
          let download = downloads.getNext().QueryInterface(Ci.nsIDownload);
          this._getOrAddDataItem(download, true);
        }
        this._loadState = this.kLoadActive;

        
        this._views.forEach(
          function (view) view.onDataLoadCompleted()
        );
        DownloadsCommon.log("Active downloads done loading.");
      }
    } else {
      if (this._loadState != this.kLoadAll) {
        
        
        
        
        DownloadsCommon.log("Loading all downloads from the persistence database.");
        let dbConnection = Services.downloads.DBConnection;
        let statement = dbConnection.createAsyncStatement(
          "SELECT guid, target, name, source, referrer, state, "
        +        "startTime, endTime, currBytes, maxBytes "
        + "FROM moz_downloads "
        + "ORDER BY startTime DESC"
        );
        try {
          this._pendingStatement = statement.executeAsync(this);
        } finally {
          statement.finalize();
        }
      }
    }
  },

  


  _terminateDataAccess: function DD_terminateDataAccess()
  {
    if (this._pendingStatement) {
      this._pendingStatement.cancel();
      this._pendingStatement = null;
    }

    
    
    Array.slice(this._views, 0).forEach(
      function (view) view.onDataInvalidated()
    );
  },

  
  

  handleResult: function DD_handleResult(aResultSet)
  {
    for (let row = aResultSet.getNextRow();
         row;
         row = aResultSet.getNextRow()) {
      
      
      
      this._getOrAddDataItem(row, false);
    }
  },

  handleError: function DD_handleError(aError)
  {
    DownloadsCommon.error("Database statement execution error (",
                          aError.result, "): ", aError.message);
  },

  handleCompletion: function DD_handleCompletion(aReason)
  {
    DownloadsCommon.log("Loading all downloads from database completed with reason:",
                        aReason);
    this._pendingStatement = null;

    
    
    
    if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
      this._loadState = this.kLoadAll;
    }

    
    
    
    
    
    this._views.forEach(
      function (view) view.onDataLoadCompleted()
    );
  },

  
  

  observe: function DD_observe(aSubject, aTopic, aData)
  {
    switch (aTopic) {
      case "download-manager-remove-download-guid":
        
        if (aSubject) {
            let downloadGuid = aSubject.data.QueryInterface(Ci.nsISupportsCString);
            DownloadsCommon.log("A single download with id",
                                downloadGuid, "was removed.");
          this._removeDataItem(downloadGuid);
          break;
        }

        
        
        DownloadsCommon.log("Multiple downloads were removed.");
        for each (let dataItem in this.dataItems) {
          if (dataItem) {
            
            
            let dataItemBinding = dataItem;
            Services.downloads.getDownloadByGUID(dataItemBinding.downloadGuid,
                                                 function(aStatus, aResult) {
              if (aStatus == Components.results.NS_ERROR_NOT_AVAILABLE) {
                DownloadsCommon.log("Removing download with id",
                                    dataItemBinding.downloadGuid);
                this._removeDataItem(dataItemBinding.downloadGuid);
              }
            }.bind(this));
          }
        }
        break;
    }
  },

  
  

  onDownloadStateChange: function DD_onDownloadStateChange(aOldState, aDownload)
  {
    if (aDownload.isPrivate != this._isPrivate) {
      
      
      return;
    }

    
    
    
    let isNew = aOldState == nsIDM.DOWNLOAD_NOTSTARTED ||
                aOldState == nsIDM.DOWNLOAD_QUEUED;

    let dataItem = this._getOrAddDataItem(aDownload, isNew);
    if (!dataItem) {
      return;
    }

    let wasInProgress = dataItem.inProgress;

    DownloadsCommon.log("A download changed its state to:", aDownload.state);
    dataItem.state = aDownload.state;
    dataItem.referrer = aDownload.referrer && aDownload.referrer.spec;
    dataItem.resumable = aDownload.resumable;
    dataItem.startTime = Math.round(aDownload.startTime / 1000);
    dataItem.currBytes = aDownload.amountTransferred;
    dataItem.maxBytes = aDownload.size;

    if (wasInProgress && !dataItem.inProgress) {
      dataItem.endTime = Date.now();
    }

    
    
    
    
    
    
    
    if (dataItem._download) {
      dataItem._download = aDownload;
    }

    for (let view of this._views) {
      try {
        view.getViewItem(dataItem).onStateChange(aOldState);
      } catch (ex) {
        Cu.reportError(ex);
      }
    }

    if (isNew && !dataItem.newDownloadNotified) {
      dataItem.newDownloadNotified = true;
      this._notifyDownloadEvent("start");
    }

    
    if (dataItem.done) {
      this._notifyDownloadEvent("finish");
    }

    
    
    if (!this._isPrivate && !dataItem.inProgress) {
      let downloadMetaData = { state: dataItem.state,
                               endTime: dataItem.endTime };
      if (dataItem.done)
        downloadMetaData.fileSize = dataItem.maxBytes;

      try {
        PlacesUtils.annotations.setPageAnnotation(
          NetUtil.newURI(dataItem.uri), "downloads/metaData", JSON.stringify(downloadMetaData), 0,
          PlacesUtils.annotations.EXPIRE_WITH_HISTORY);
      }
      catch(ex) {
        Cu.reportError(ex);
      }
    }
  },

  onProgressChange: function DD_onProgressChange(aWebProgress, aRequest,
                                                  aCurSelfProgress,
                                                  aMaxSelfProgress,
                                                  aCurTotalProgress,
                                                  aMaxTotalProgress, aDownload)
  {
    if (aDownload.isPrivate != this._isPrivate) {
      
      
      return;
    }

    let dataItem = this._getOrAddDataItem(aDownload, false);
    if (!dataItem) {
      return;
    }

    dataItem.currBytes = aDownload.amountTransferred;
    dataItem.maxBytes = aDownload.size;
    dataItem.speed = aDownload.speed;
    dataItem.percentComplete = aDownload.percentComplete;

    this._views.forEach(
      function (view) view.getViewItem(dataItem).onProgressChange()
    );
  },

  onStateChange: function () { },

  onSecurityChange: function () { },

  
  

  



  get panelHasShownBefore() {
    try {
      return Services.prefs.getBoolPref("browser.download.panel.shown");
    } catch (ex) { }
    return false;
  },

  set panelHasShownBefore(aValue) {
    Services.prefs.setBoolPref("browser.download.panel.shown", aValue);
    return aValue;
  },

  






  _notifyDownloadEvent: function DD_notifyDownloadEvent(aType)
  {
    DownloadsCommon.log("Attempting to notify that a new download has started or finished.");
    if (DownloadsCommon.useToolkitUI) {
      DownloadsCommon.log("Cancelling notification - we're using the toolkit downloads manager.");
      return;
    }

    
    let browserWin = RecentWindow.getMostRecentBrowserWindow({ private: this._isPrivate });
    if (!browserWin) {
      return;
    }

    if (this.panelHasShownBefore) {
      
      
      
      DownloadsCommon.log("Showing new download notification.");
      browserWin.DownloadsIndicatorView.showEventNotification(aType);
      return;
    }
    this.panelHasShownBefore = true;
    browserWin.DownloadsPanel.showPanel();
  }
};

XPCOMUtils.defineLazyGetter(this, "PrivateDownloadsData", function() {
  return new DownloadsDataCtor(true);
});

XPCOMUtils.defineLazyGetter(this, "DownloadsData", function() {
  return new DownloadsDataCtor(false);
});














function DownloadsDataItem(aSource)
{
  if (aSource instanceof Ci.nsIDownload) {
    this._initFromDownload(aSource);
  } else {
    this._initFromDataRow(aSource);
  }
}

DownloadsDataItem.prototype = {
  







  _initFromDownload: function DDI_initFromDownload(aDownload)
  {
    this._download = aDownload;

    
    this.downloadGuid = aDownload.guid;
    this.file = aDownload.target.spec;
    this.target = aDownload.displayName;
    this.uri = aDownload.source.spec;
    this.referrer = aDownload.referrer && aDownload.referrer.spec;
    this.state = aDownload.state;
    this.startTime = Math.round(aDownload.startTime / 1000);
    this.endTime = Date.now();
    this.currBytes = aDownload.amountTransferred;
    this.maxBytes = aDownload.size;
    this.resumable = aDownload.resumable;
    this.speed = aDownload.speed;
    this.percentComplete = aDownload.percentComplete;
  },

  











  _initFromDataRow: function DDI_initFromDataRow(aStorageRow)
  {
    
    this._download = null;
    this.downloadGuid = aStorageRow.getResultByName("guid");
    this.file = aStorageRow.getResultByName("target");
    this.target = aStorageRow.getResultByName("name");
    this.uri = aStorageRow.getResultByName("source");
    this.referrer = aStorageRow.getResultByName("referrer");
    this.state = aStorageRow.getResultByName("state");
    this.startTime = Math.round(aStorageRow.getResultByName("startTime") / 1000);
    this.endTime = Math.round(aStorageRow.getResultByName("endTime") / 1000);
    this.currBytes = aStorageRow.getResultByName("currBytes");
    this.maxBytes = aStorageRow.getResultByName("maxBytes");

    
    
    
    
    
    
    

    
    
    this.resumable = true;

    if (this.state == nsIDM.DOWNLOAD_DOWNLOADING) {
      this.getDownload(function(aDownload) {
        this.resumable = aDownload.resumable;
      }.bind(this));
    }

    
    this.speed = 0;
    this.percentComplete = this.maxBytes <= 0
                           ? -1
                           : Math.round(this.currBytes / this.maxBytes * 100);
  },

  







  getDownload: function DDI_getDownload(aCallback) {
    if (this._download) {
      
      let download = this._download;
      Services.tm.mainThread.dispatch(function () aCallback(download),
                                      Ci.nsIThread.DISPATCH_NORMAL);
    } else {
      Services.downloads.getDownloadByGUID(this.downloadGuid,
                                           function(aStatus, aResult) {
        if (!Components.isSuccessCode(aStatus)) {
          Cu.reportError(
            new Components.Exception("Cannot retrieve download for GUID: " +
                                     this.downloadGuid));
        } else {
          this._download = aResult;
          aCallback(aResult);
        }
      }.bind(this));
    }
  },

  




  get inProgress()
  {
    return [
      nsIDM.DOWNLOAD_NOTSTARTED,
      nsIDM.DOWNLOAD_QUEUED,
      nsIDM.DOWNLOAD_DOWNLOADING,
      nsIDM.DOWNLOAD_PAUSED,
      nsIDM.DOWNLOAD_SCANNING,
    ].indexOf(this.state) != -1;
  },

  



  get starting()
  {
    return this.state == nsIDM.DOWNLOAD_NOTSTARTED ||
           this.state == nsIDM.DOWNLOAD_QUEUED;
  },

  


  get paused()
  {
    return this.state == nsIDM.DOWNLOAD_PAUSED;
  },

  



  get done()
  {
    return [
      nsIDM.DOWNLOAD_FINISHED,
      nsIDM.DOWNLOAD_BLOCKED_PARENTAL,
      nsIDM.DOWNLOAD_BLOCKED_POLICY,
      nsIDM.DOWNLOAD_DIRTY,
    ].indexOf(this.state) != -1;
  },

  


  get openable()
  {
    return this.state == nsIDM.DOWNLOAD_FINISHED;
  },

  



  get canRetry()
  {
    return this.state == nsIDM.DOWNLOAD_CANCELED ||
           this.state == nsIDM.DOWNLOAD_FAILED;
  },

  






  get localFile()
  {
    return this._getFile(this.file);
  },

  






  get partFile()
  {
    return this._getFile(this.file + kPartialDownloadSuffix);
  },

  











  _getFile: function DDI__getFile(aFilename)
  {
    
    
    
    if (aFilename.startsWith("file:")) {
      
      
      let fileUrl = NetUtil.newURI(aFilename).QueryInterface(Ci.nsIFileURL);
      return fileUrl.file.clone().QueryInterface(Ci.nsILocalFile);
    } else {
      
      
      return new DownloadsLocalFileCtor(aFilename);
    }
  },

  






  openLocalFile: function DDI_openLocalFile(aOwnerWindow) {
    this.getDownload(function(aDownload) {
      DownloadsCommon.openDownloadedFile(this.localFile,
                                         aDownload.MIMEInfo,
                                         aOwnerWindow);
    }.bind(this));
  },

  


  showLocalFile: function DDI_showLocalFile() {
    DownloadsCommon.showDownloadedFile(this.localFile);
  },

  



  togglePauseResume: function DDI_togglePauseResume() {
    if (!this.inProgress || !this.resumable)
      throw new Error("The given download cannot be paused or resumed");

    this.getDownload(function(aDownload) {
      if (this.inProgress) {
        if (this.paused)
          aDownload.resume();
        else
          aDownload.pause();
      }
    }.bind(this));
  },

  



  retry: function DDI_retry() {
    if (!this.canRetry)
      throw new Error("Cannot rerty this download");

    this.getDownload(function(aDownload) {
      aDownload.retry();
    });
  },

  




  _ensureLocalFileRemoved: function DDI__ensureLocalFileRemoved()
  {
    try {
      let localFile = this.localFile;
      if (localFile.exists()) {
        localFile.remove(false);
      }
    } catch (ex) { }
  },

  



  cancel: function() {
    if (!this.inProgress)
      throw new Error("Cannot cancel this download");

    this.getDownload(function (aDownload) {
      aDownload.cancel();
      this._ensureLocalFileRemoved();
    }.bind(this));
  },

  


  remove: function DDI_remove() {
    this.getDownload(function (aDownload) {
      if (this.inProgress) {
        aDownload.cancel();
        this._ensureLocalFileRemoved();
      }
      aDownload.remove();
    }.bind(this));
  }
};








const DownloadsViewPrototype = {
  
  

  





  _views: null,

  





  _isPrivate: false,

  







  addView: function DVP_addView(aView)
  {
    
    if (this._views.length == 0) {
      if (this._isPrivate) {
        PrivateDownloadsData.addView(this);
      } else {
        DownloadsData.addView(this);
      }
    }

    this._views.push(aView);
    this.refreshView(aView);
  },

  





  refreshView: function DVP_refreshView(aView)
  {
    
    
    this._refreshProperties();
    this._updateView(aView);
  },

  





  removeView: function DVP_removeView(aView)
  {
    let index = this._views.indexOf(aView);
    if (index != -1) {
      this._views.splice(index, 1);
    }

    
    if (this._views.length == 0) {
      if (this._isPrivate) {
        PrivateDownloadsData.removeView(this);
      } else {
        DownloadsData.removeView(this);
      }
    }
  },

  
  

  


  _loading: false,

  


  onDataLoadStarting: function DVP_onDataLoadStarting()
  {
    this._loading = true;
  },

  


  onDataLoadCompleted: function DVP_onDataLoadCompleted()
  {
    this._loading = false;
  },

  






  onDataInvalidated: function DVP_onDataInvalidated()
  {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  














  onDataItemAdded: function DVP_onDataItemAdded(aDataItem, aNewest)
  {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  








  onDataItemRemoved: function DVP_onDataItemRemoved(aDataItem)
  {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  









  getViewItem: function DID_getViewItem(aDataItem)
  {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  





  _refreshProperties: function DID_refreshProperties()
  {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  




  _updateView: function DID_updateView()
  {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  }
};














function DownloadsIndicatorDataCtor(aPrivate) {
  this._isPrivate = aPrivate;
  this._views = [];
}
DownloadsIndicatorDataCtor.prototype = {
  __proto__: DownloadsViewPrototype,

  





  removeView: function DID_removeView(aView)
  {
    DownloadsViewPrototype.removeView.call(this, aView);

    if (this._views.length == 0) {
      this._itemCount = 0;
    }
  },

  
  

  


  onDataLoadCompleted: function DID_onDataLoadCompleted()
  {
    DownloadsViewPrototype.onDataLoadCompleted.call(this);
    this._updateViews();
  },

  




  onDataInvalidated: function DID_onDataInvalidated()
  {
    this._itemCount = 0;
  },

  












  onDataItemAdded: function DID_onDataItemAdded(aDataItem, aNewest)
  {
    this._itemCount++;
    this._updateViews();
  },

  






  onDataItemRemoved: function DID_onDataItemRemoved(aDataItem)
  {
    this._itemCount--;
    this._updateViews();
  },

  







  getViewItem: function DID_getViewItem(aDataItem)
  {
    let data = this._isPrivate ? PrivateDownloadsIndicatorData
                               : DownloadsIndicatorData;
    return Object.freeze({
      onStateChange: function DIVI_onStateChange(aOldState)
      {
        if (aDataItem.state == nsIDM.DOWNLOAD_FINISHED ||
            aDataItem.state == nsIDM.DOWNLOAD_FAILED) {
          data.attention = true;
        }

        
        data._lastRawTimeLeft = -1;
        data._lastTimeLeft = -1;

        data._updateViews();
      },
      onProgressChange: function DIVI_onProgressChange()
      {
        data._updateViews();
      }
    });
  },

  
  

  
  
  _hasDownloads: false,
  _counter: "",
  _percentComplete: -1,
  _paused: false,

  


  set attention(aValue)
  {
    this._attention = aValue;
    this._updateViews();
    return aValue;
  },
  _attention: false,

  



  set attentionSuppressed(aValue)
  {
    this._attentionSuppressed = aValue;
    this._attention = false;
    this._updateViews();
    return aValue;
  },
  _attentionSuppressed: false,

  


  _updateViews: function DID_updateViews()
  {
    
    if (this._loading) {
      return;
    }

    this._refreshProperties();
    this._views.forEach(this._updateView, this);
  },

  





  _updateView: function DID_updateView(aView)
  {
    aView.hasDownloads = this._hasDownloads;
    aView.counter = this._counter;
    aView.percentComplete = this._percentComplete;
    aView.paused = this._paused;
    aView.attention = this._attention && !this._attentionSuppressed;
  },

  
  

  


  _itemCount: 0,

  





  _lastRawTimeLeft: -1,

  





  _lastTimeLeft: -1,

  





  _activeDataItems: function DID_activeDataItems()
  {
    let dataItems = this._isPrivate ? PrivateDownloadsData.dataItems
                                    : DownloadsData.dataItems;
    for each (let dataItem in dataItems) {
      if (dataItem && dataItem.inProgress) {
        yield dataItem;
      }
    }
  },

  


  _refreshProperties: function DID_refreshProperties()
  {
    let summary =
      DownloadsCommon.summarizeDownloads(this._activeDataItems());

    
    this._hasDownloads = (this._itemCount > 0);

    
    this._paused = summary.numActive > 0 &&
                   summary.numActive == summary.numPaused;

    this._percentComplete = summary.percentComplete;

    
    if (summary.rawTimeLeft == -1) {
      
      this._lastRawTimeLeft = -1;
      this._lastTimeLeft = -1;
      this._counter = "";
    } else {
      
      if (this._lastRawTimeLeft != summary.rawTimeLeft) {
        this._lastRawTimeLeft = summary.rawTimeLeft;
        this._lastTimeLeft = DownloadsCommon.smoothSeconds(summary.rawTimeLeft,
                                                           this._lastTimeLeft);
      }
      this._counter = DownloadsCommon.formatTimeLeft(this._lastTimeLeft);
    }
  }
};

XPCOMUtils.defineLazyGetter(this, "PrivateDownloadsIndicatorData", function() {
  return new DownloadsIndicatorDataCtor(true);
});

XPCOMUtils.defineLazyGetter(this, "DownloadsIndicatorData", function() {
  return new DownloadsIndicatorDataCtor(false);
});


















function DownloadsSummaryData(aIsPrivate, aNumToExclude) {
  this._numToExclude = aNumToExclude;
  
  
  
  this._loading = false;

  this._dataItems = [];

  
  
  
  
  this._lastRawTimeLeft = -1;

  
  
  
  
  this._lastTimeLeft = -1;

  
  
  this._showingProgress = false;
  this._details = "";
  this._description = "";
  this._numActive = 0;
  this._percentComplete = -1;

  this._isPrivate = aIsPrivate;
  this._views = [];
}

DownloadsSummaryData.prototype = {
  __proto__: DownloadsViewPrototype,

  





  removeView: function DSD_removeView(aView)
  {
    DownloadsViewPrototype.removeView.call(this, aView);

    if (this._views.length == 0) {
      
      
      this._dataItems = [];
    }
  },

  
  
  
  

  onDataLoadCompleted: function DSD_onDataLoadCompleted()
  {
    DownloadsViewPrototype.onDataLoadCompleted.call(this);
    this._updateViews();
  },

  onDataInvalidated: function DSD_onDataInvalidated()
  {
    this._dataItems = [];
  },

  onDataItemAdded: function DSD_onDataItemAdded(aDataItem, aNewest)
  {
    if (aNewest) {
      this._dataItems.unshift(aDataItem);
    } else {
      this._dataItems.push(aDataItem);
    }

    this._updateViews();
  },

  onDataItemRemoved: function DSD_onDataItemRemoved(aDataItem)
  {
    let itemIndex = this._dataItems.indexOf(aDataItem);
    this._dataItems.splice(itemIndex, 1);
    this._updateViews();
  },

  getViewItem: function DSD_getViewItem(aDataItem)
  {
    let self = this;
    return Object.freeze({
      onStateChange: function DIVI_onStateChange(aOldState)
      {
        
        self._lastRawTimeLeft = -1;
        self._lastTimeLeft = -1;
        self._updateViews();
      },
      onProgressChange: function DIVI_onProgressChange()
      {
        self._updateViews();
      }
    });
  },

  
  

  


  _updateViews: function DSD_updateViews()
  {
    
    if (this._loading) {
      return;
    }

    this._refreshProperties();
    this._views.forEach(this._updateView, this);
  },

  





  _updateView: function DSD_updateView(aView)
  {
    aView.showingProgress = this._showingProgress;
    aView.percentComplete = this._percentComplete;
    aView.description = this._description;
    aView.details = this._details;
  },

  
  

  






  _dataItemsForSummary: function DSD_dataItemsForSummary()
  {
    if (this._dataItems.length > 0) {
      for (let i = this._numToExclude; i < this._dataItems.length; ++i) {
        yield this._dataItems[i];
      }
    }
  },

  


  _refreshProperties: function DSD_refreshProperties()
  {
    
    let summary =
      DownloadsCommon.summarizeDownloads(this._dataItemsForSummary());

    this._description = DownloadsCommon.strings
                                       .otherDownloads2(summary.numActive);
    this._percentComplete = summary.percentComplete;

    
    this._showingProgress = summary.numDownloading > 0 ||
                            summary.numPaused > 0;

    
    if (summary.rawTimeLeft == -1) {
      
      this._lastRawTimeLeft = -1;
      this._lastTimeLeft = -1;
      this._details = "";
    } else {
      
      if (this._lastRawTimeLeft != summary.rawTimeLeft) {
        this._lastRawTimeLeft = summary.rawTimeLeft;
        this._lastTimeLeft = DownloadsCommon.smoothSeconds(summary.rawTimeLeft,
                                                           this._lastTimeLeft);
      }
      [this._details] = DownloadUtils.getDownloadStatusNoRate(
        summary.totalTransferred, summary.totalSize, summary.slowestSpeed,
        this._lastTimeLeft);
    }
  }
}
