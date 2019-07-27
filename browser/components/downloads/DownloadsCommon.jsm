





"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadsCommon",
  "DownloadsDataItem",
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
XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadUIHelper",
                                  "resource://gre/modules/DownloadUIHelper.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadUtils",
                                  "resource://gre/modules/DownloadUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm")
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
                                  "resource:///modules/RecentWindow.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadsLogger",
                                  "resource:///modules/DownloadsLogger.jsm");

const nsIDM = Ci.nsIDownloadManager;

const kDownloadsStringBundleUrl =
  "chrome://browser/locale/downloads/downloads.properties";

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

const kPartialDownloadSuffix = ".part";

const kPrefBranch = Services.prefs.getBranch("browser.download.");

let PrefObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),
  getPref(name) {
    try {
      switch (typeof this.prefs[name]) {
        case "boolean":
          return kPrefBranch.getBoolPref(name);
      }
    } catch (ex) { }
    return this.prefs[name];
  },
  observe(aSubject, aTopic, aData) {
    if (this.prefs.hasOwnProperty(aData)) {
      delete this[aData];
      return this[aData] = this.getPref(aData);
    }
  },
  register(prefs) {
    this.prefs = prefs;
    kPrefBranch.addObserver("", this, true);
    for (let key in prefs) {
      let name = key;
      XPCOMUtils.defineLazyGetter(this, name, function () {
        return PrefObserver.getPref(name);
      });
    }
  },
};

PrefObserver.register({
  
  debug: false,
  animateNotifications: true
});









this.DownloadsCommon = {
  


  BLOCK_VERDICT_MALWARE: "Malware",
  BLOCK_VERDICT_POTENTIALLY_UNWANTED: "PotentiallyUnwanted",
  BLOCK_VERDICT_UNCOMMON: "Uncommon",

  log(...aMessageArgs) {
    if (!PrefObserver.debug) {
      return;
    }
    DownloadsLogger.log(...aMessageArgs);
  },

  error(...aMessageArgs) {
    if (!PrefObserver.debug) {
      return;
    }
    DownloadsLogger.reportError(...aMessageArgs);
  },

  




  get strings() {
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

  









  formatTimeLeft(aSeconds) {
    
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

  



  get animateNotifications() {
    return PrefObserver.animateNotifications;
  },

  






  getData(aWindow) {
    if (PrivateBrowsingUtils.isWindowPrivate(aWindow)) {
      return PrivateDownloadsData;
    } else {
      return DownloadsData;
    }
  },

  



  initializeAllDataLinks() {
    DownloadsData.initializeDataLink();
    PrivateDownloadsData.initializeDataLink();
  },

  




  getIndicatorData(aWindow) {
    if (PrivateBrowsingUtils.isWindowPrivate(aWindow)) {
      return PrivateDownloadsIndicatorData;
    } else {
      return DownloadsIndicatorData;
    }
  },

  









  getSummary(aWindow, aNumToExclude) {
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

  




















  summarizeDownloads(aDataItems) {
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
          if (dataItem.maxBytes > 0 && dataItem.download.speed > 0) {
            let sizeLeft = dataItem.maxBytes - dataItem.download.currentBytes;
            summary.rawTimeLeft = Math.max(summary.rawTimeLeft,
                                           sizeLeft / dataItem.download.speed);
            summary.slowestSpeed = Math.min(summary.slowestSpeed,
                                            dataItem.download.speed);
          }
          break;
      }
      
      if (dataItem.maxBytes > 0 &&
          dataItem.state != nsIDM.DOWNLOAD_CANCELED &&
          dataItem.state != nsIDM.DOWNLOAD_FAILED) {
        summary.totalSize += dataItem.maxBytes;
        summary.totalTransferred += dataItem.download.currentBytes;
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

  








  smoothSeconds(aSeconds, aLastSeconds) {
    
    
    
    let shouldApplySmoothing = aLastSeconds >= 0 &&
                               aSeconds > aLastSeconds / 2;
    if (shouldApplySmoothing) {
      
      
      let diff = aSeconds - aLastSeconds;
      aSeconds = aLastSeconds + (diff < 0 ? .3 : .1) * diff;

      
      
      diff = aSeconds - aLastSeconds;
      let diffPercent = diff / aLastSeconds * 100;
      if (Math.abs(diff) < 5 || Math.abs(diffPercent) < 5) {
        aSeconds = aLastSeconds - (diff < 0 ? .4 : .2);
      }
    }

    
    
    
    return aLastSeconds = Math.max(aSeconds, 1);
  },

  









  openDownloadedFile(aFile, aMimeInfo, aOwnerWindow) {
    if (!(aFile instanceof Ci.nsIFile)) {
      throw new Error("aFile must be a nsIFile object");
    }
    if (aMimeInfo && !(aMimeInfo instanceof Ci.nsIMIMEInfo)) {
      throw new Error("Invalid value passed for aMimeInfo");
    }
    if (!(aOwnerWindow instanceof Ci.nsIDOMWindow)) {
      throw new Error("aOwnerWindow must be a dom-window object");
    }

    let promiseShouldLaunch;
    if (aFile.isExecutable()) {
      
      
      promiseShouldLaunch =
        DownloadUIHelper.getPrompter(aOwnerWindow)
                        .confirmLaunchExecutable(aFile.path);
    } else {
      promiseShouldLaunch = Promise.resolve(true);
    }

    promiseShouldLaunch.then(shouldLaunch => {
      if (!shouldLaunch) {
        return;
      }
  
      
      try {
        if (aMimeInfo && aMimeInfo.preferredAction == aMimeInfo.useHelperApp) {
          aMimeInfo.launchWithFile(aFile);
          return;
        }
      } catch (ex) { }
  
      
      
      try {
        aFile.launch();
      } catch (ex) {
        
        
        Cc["@mozilla.org/uriloader/external-protocol-service;1"]
          .getService(Ci.nsIExternalProtocolService)
          .loadUrl(NetUtil.newURI(aFile));
      }
    }).then(null, Cu.reportError);
  },

  





  showDownloadedFile(aFile) {
    if (!(aFile instanceof Ci.nsIFile)) {
      throw new Error("aFile must be a nsIFile object");
    }
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
  },

  











  confirmUnblockDownload: Task.async(function* (aType, aOwnerWindow) {
    let s = DownloadsCommon.strings;
    let title = s.unblockHeader;
    let buttonFlags = (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_0) +
                      (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_1);
    let type = "";
    let message = s.unblockTip;
    let okButton = s.unblockButtonContinue;
    let cancelButton = s.unblockButtonCancel;

    switch (aType) {
      case this.BLOCK_VERDICT_MALWARE:
        type = s.unblockTypeMalware;
        break;
      case this.BLOCK_VERDICT_POTENTIALLY_UNWANTED:
        type = s.unblockTypePotentiallyUnwanted;
        break;
      case this.BLOCK_VERDICT_UNCOMMON:
        type = s.unblockTypeUncommon;
        break;
    }

    if (type) {
      message = type + "\n\n" + message;
    }

    Services.ww.registerNotification(function onOpen(subj, topic) {
      if (topic == "domwindowopened" && subj instanceof Ci.nsIDOMWindow) {
        
        
        subj.addEventListener("DOMContentLoaded", function onLoad() {
          subj.removeEventListener("DOMContentLoaded", onLoad);
          if (subj.document.documentURI ==
              "chrome://global/content/commonDialog.xul") {
            Services.ww.unregisterNotification(onOpen);
            let dialog = subj.document.getElementById("commonDialog");
            if (dialog) {
              
              dialog.classList.add("alert-dialog");
            }
          }
        });
      }
    });

    let rv = Services.prompt.confirmEx(aOwnerWindow, title, message, buttonFlags,
                                       cancelButton, okButton, null, null, {});
    return (rv == 1);
  }),
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

  
  this.dataItems = new Set();

  
  
  this._views = [];

  
  this._downloadToDataItemMap = new Map();
}

DownloadsDataCtor.prototype = {
  


  initializeDataLink() {
    if (!this._dataLinkInitialized) {
      let promiseList = Downloads.getList(this._isPrivate ? Downloads.PRIVATE
                                                          : Downloads.PUBLIC);
      promiseList.then(list => list.addView(this)).then(null, Cu.reportError);
      this._dataLinkInitialized = true;
    }
  },
  _dataLinkInitialized: false,

  


  get canRemoveFinished() {
    for (let dataItem of this.dataItems) {
      if (!dataItem.inProgress) {
        return true;
      }
    }
    return false;
  },

  


  removeFinished() {
    let promiseList = Downloads.getList(this._isPrivate ? Downloads.PRIVATE
                                                        : Downloads.PUBLIC);
    promiseList.then(list => list.removeFinished())
               .then(null, Cu.reportError);
  },

  
  

  onDownloadAdded(aDownload) {
    let dataItem = new DownloadsDataItem(aDownload);
    this._downloadToDataItemMap.set(aDownload, dataItem);
    this.dataItems.add(dataItem);

    for (let view of this._views) {
      view.onDataItemAdded(dataItem, true);
    }

    this._updateDataItemState(dataItem);
  },

  onDownloadChanged(aDownload) {
    let dataItem = this._downloadToDataItemMap.get(aDownload);
    if (!dataItem) {
      Cu.reportError("Download doesn't exist.");
      return;
    }

    this._updateDataItemState(dataItem);
  },

  onDownloadRemoved(aDownload) {
    let dataItem = this._downloadToDataItemMap.get(aDownload);
    if (!dataItem) {
      Cu.reportError("Download doesn't exist.");
      return;
    }

    this._downloadToDataItemMap.delete(aDownload);
    this.dataItems.delete(dataItem);
    for (let view of this._views) {
      view.onDataItemRemoved(dataItem);
    }
  },

  


  _updateDataItemState(aDataItem) {
    let oldState = aDataItem.state;
    let wasInProgress = aDataItem.inProgress;
    let wasDone = aDataItem.done;

    aDataItem.updateFromDownload();

    if (wasInProgress && !aDataItem.inProgress) {
      aDataItem.endTime = Date.now();
    }

    if (oldState != aDataItem.state) {
      for (let view of this._views) {
        try {
          view.onDataItemStateChanged(aDataItem, oldState);
        } catch (ex) {
          Cu.reportError(ex);
        }
      }

      
      
      
      
      if (!this._isPrivate && !aDataItem.inProgress) {
        try {
          let downloadMetaData = { state: aDataItem.state,
                                   endTime: aDataItem.endTime };
          if (aDataItem.done) {
            downloadMetaData.fileSize = aDataItem.maxBytes;
          }

          PlacesUtils.annotations.setPageAnnotation(
                        NetUtil.newURI(aDataItem.download.source.url),
                        "downloads/metaData",
                        JSON.stringify(downloadMetaData), 0,
                        PlacesUtils.annotations.EXPIRE_WITH_HISTORY);
        } catch (ex) {
          Cu.reportError(ex);
        }
      }
    }

    if (!aDataItem.newDownloadNotified) {
      aDataItem.newDownloadNotified = true;
      this._notifyDownloadEvent("start");
    }

    if (!wasDone && aDataItem.done) {
      this._notifyDownloadEvent("finish");
    }

    for (let view of this._views) {
      view.onDataItemChanged(aDataItem);
    }
  },

  
  

  







  addView(aView) {
    this._views.push(aView);
    this._updateView(aView);
  },

  





  removeView(aView) {
    let index = this._views.indexOf(aView);
    if (index != -1) {
      this._views.splice(index, 1);
    }
  },

  





  _updateView(aView) {
    
    aView.onDataLoadStarting();

    
    
    let loadedItemsArray = [...this.dataItems];
    loadedItemsArray.sort((a, b) => b.download.startTime - a.download.startTime);
    loadedItemsArray.forEach(dataItem => aView.onDataItemAdded(dataItem, false));

    
    aView.onDataLoadCompleted();
  },

  
  

  



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

  






  _notifyDownloadEvent(aType) {
    DownloadsCommon.log("Attempting to notify that a new download has started or finished.");

    
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












function DownloadsDataItem(aDownload) {
  this.download = aDownload;
  this.endTime = Date.now();
  this.updateFromDownload();
}

DownloadsDataItem.prototype = {
  


  updateFromDownload() {
    
    if (this.download.succeeded) {
      this.state = nsIDM.DOWNLOAD_FINISHED;
    } else if (this.download.error &&
               this.download.error.becauseBlockedByParentalControls) {
      this.state = nsIDM.DOWNLOAD_BLOCKED_PARENTAL;
    } else if (this.download.error &&
               this.download.error.becauseBlockedByReputationCheck) {
      this.state = nsIDM.DOWNLOAD_DIRTY;
    } else if (this.download.error) {
      this.state = nsIDM.DOWNLOAD_FAILED;
    } else if (this.download.canceled && this.download.hasPartialData) {
      this.state = nsIDM.DOWNLOAD_PAUSED;
    } else if (this.download.canceled) {
      this.state = nsIDM.DOWNLOAD_CANCELED;
    } else if (this.download.stopped) {
      this.state = nsIDM.DOWNLOAD_NOTSTARTED;
    } else {
      this.state = nsIDM.DOWNLOAD_DOWNLOADING;
    }

    if (this.download.succeeded) {
      
      
      
      this.maxBytes = this.download.hasProgress ?
                             this.download.totalBytes :
                             this.download.currentBytes;
      this.percentComplete = 100;
    } else if (this.download.hasProgress) {
      
      this.maxBytes = this.download.totalBytes;
      this.percentComplete = this.download.progress;
    } else {
      
      this.maxBytes = -1;
      this.percentComplete = -1;
    }
  },

  




  get inProgress() {
    return [
      nsIDM.DOWNLOAD_NOTSTARTED,
      nsIDM.DOWNLOAD_QUEUED,
      nsIDM.DOWNLOAD_DOWNLOADING,
      nsIDM.DOWNLOAD_PAUSED,
      nsIDM.DOWNLOAD_SCANNING,
    ].indexOf(this.state) != -1;
  },

  



  get starting() {
    return this.state == nsIDM.DOWNLOAD_NOTSTARTED ||
           this.state == nsIDM.DOWNLOAD_QUEUED;
  },

  


  get paused() {
    return this.state == nsIDM.DOWNLOAD_PAUSED;
  },

  



  get done() {
    return [
      nsIDM.DOWNLOAD_FINISHED,
      nsIDM.DOWNLOAD_BLOCKED_PARENTAL,
      nsIDM.DOWNLOAD_BLOCKED_POLICY,
      nsIDM.DOWNLOAD_DIRTY,
    ].indexOf(this.state) != -1;
  },

  



  get canRetry() {
    return this.state == nsIDM.DOWNLOAD_CANCELED ||
           this.state == nsIDM.DOWNLOAD_FAILED;
  },

  








  get localFile() {
    
    return new FileUtils.File(this.download.target.path);
  },

  








  get partFile() {
    return new FileUtils.File(this.download.target.path + kPartialDownloadSuffix);
  },
};








const DownloadsViewPrototype = {
  
  

  





  _views: null,

  





  _isPrivate: false,

  







  addView(aView) {
    
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

  





  refreshView(aView) {
    
    
    this._refreshProperties();
    this._updateView(aView);
  },

  





  removeView(aView) {
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

  


  onDataLoadStarting() {
    this._loading = true;
  },

  


  onDataLoadCompleted() {
    this._loading = false;
  },

  














  onDataItemAdded(aDataItem, aNewest) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  








  onDataItemRemoved(aDataItem) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  






  onDataItemStateChanged(aDataItem) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  








  onDataItemChanged(aDataItem) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  





  _refreshProperties() {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  




  _updateView() {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },
};














function DownloadsIndicatorDataCtor(aPrivate) {
  this._isPrivate = aPrivate;
  this._views = [];
}
DownloadsIndicatorDataCtor.prototype = {
  __proto__: DownloadsViewPrototype,

  





  removeView(aView) {
    DownloadsViewPrototype.removeView.call(this, aView);

    if (this._views.length == 0) {
      this._itemCount = 0;
    }
  },

  
  

  


  onDataLoadCompleted() {
    DownloadsViewPrototype.onDataLoadCompleted.call(this);
    this._updateViews();
  },

  












  onDataItemAdded(aDataItem, aNewest) {
    this._itemCount++;
    this._updateViews();
  },

  






  onDataItemRemoved(aDataItem) {
    this._itemCount--;
    this._updateViews();
  },

  
  onDataItemStateChanged(aDataItem, aOldState) {
    if (aDataItem.state == nsIDM.DOWNLOAD_FINISHED ||
        aDataItem.state == nsIDM.DOWNLOAD_FAILED) {
      this.attention = true;
    }

    
    this._lastRawTimeLeft = -1;
    this._lastTimeLeft = -1;
  },

  
  onDataItemChanged() {
    this._updateViews();
  },

  
  

  
  
  _hasDownloads: false,
  _counter: "",
  _percentComplete: -1,
  _paused: false,

  


  set attention(aValue) {
    this._attention = aValue;
    this._updateViews();
    return aValue;
  },
  _attention: false,

  



  set attentionSuppressed(aValue) {
    this._attentionSuppressed = aValue;
    this._attention = false;
    this._updateViews();
    return aValue;
  },
  _attentionSuppressed: false,

  


  _updateViews() {
    
    if (this._loading) {
      return;
    }

    this._refreshProperties();
    this._views.forEach(this._updateView, this);
  },

  





  _updateView(aView) {
    aView.hasDownloads = this._hasDownloads;
    aView.counter = this._counter;
    aView.percentComplete = this._percentComplete;
    aView.paused = this._paused;
    aView.attention = this._attention && !this._attentionSuppressed;
  },

  
  

  


  _itemCount: 0,

  





  _lastRawTimeLeft: -1,

  





  _lastTimeLeft: -1,

  





  _activeDataItems() {
    let dataItems = this._isPrivate ? PrivateDownloadsData.dataItems
                                    : DownloadsData.dataItems;
    for (let dataItem of dataItems) {
      if (dataItem && dataItem.inProgress) {
        yield dataItem;
      }
    }
  },

  


  _refreshProperties() {
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

  





  removeView(aView) {
    DownloadsViewPrototype.removeView.call(this, aView);

    if (this._views.length == 0) {
      
      
      this._dataItems = [];
    }
  },

  
  
  
  

  onDataLoadCompleted() {
    DownloadsViewPrototype.onDataLoadCompleted.call(this);
    this._updateViews();
  },

  onDataItemAdded(aDataItem, aNewest) {
    if (aNewest) {
      this._dataItems.unshift(aDataItem);
    } else {
      this._dataItems.push(aDataItem);
    }

    this._updateViews();
  },

  onDataItemRemoved(aDataItem) {
    let itemIndex = this._dataItems.indexOf(aDataItem);
    this._dataItems.splice(itemIndex, 1);
    this._updateViews();
  },

  
  onDataItemStateChanged(aOldState) {
    
    this._lastRawTimeLeft = -1;
    this._lastTimeLeft = -1;
  },

  
  onDataItemChanged() {
    this._updateViews();
  },

  
  

  


  _updateViews() {
    
    if (this._loading) {
      return;
    }

    this._refreshProperties();
    this._views.forEach(this._updateView, this);
  },

  





  _updateView(aView) {
    aView.showingProgress = this._showingProgress;
    aView.percentComplete = this._percentComplete;
    aView.description = this._description;
    aView.details = this._details;
  },

  
  

  






  _dataItemsForSummary() {
    if (this._dataItems.length > 0) {
      for (let i = this._numToExclude; i < this._dataItems.length; ++i) {
        yield this._dataItems[i];
      }
    }
  },

  


  _refreshProperties() {
    
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
  },
}
