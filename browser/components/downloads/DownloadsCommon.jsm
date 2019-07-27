





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

  


  stateOfDownload(download) {
    
    if (!download.stopped) {
      return nsIDM.DOWNLOAD_DOWNLOADING;
    }
    if (download.succeeded) {
      return nsIDM.DOWNLOAD_FINISHED;
    }
    if (download.error) {
      if (download.error.becauseBlockedByParentalControls) {
        return nsIDM.DOWNLOAD_BLOCKED_PARENTAL;
      }
      if (download.error.becauseBlockedByReputationCheck) {
        return nsIDM.DOWNLOAD_DIRTY;
      }
      return nsIDM.DOWNLOAD_FAILED;
    }
    if (download.canceled) {
      if (download.hasPartialData) {
        return nsIDM.DOWNLOAD_PAUSED;
      }
      return nsIDM.DOWNLOAD_CANCELED;
    }
    return nsIDM.DOWNLOAD_NOTSTARTED;
  },

  



  removeAndFinalizeDownload(download) {
    Downloads.getList(Downloads.ALL)
             .then(list => list.remove(download))
             .then(() => download.finalize(true))
             .catch(Cu.reportError);
  },

  



















  summarizeDownloads(downloads) {
    let summary = {
      numActive: 0,
      numPaused: 0,
      numDownloading: 0,
      totalSize: 0,
      totalTransferred: 0,
      
      
      
      
      slowestSpeed: Infinity,
      rawTimeLeft: -1,
      percentComplete: -1
    }

    for (let download of downloads) {
      summary.numActive++;

      if (!download.stopped) {
        summary.numDownloading++;
        if (download.hasProgress && download.speed > 0) {
          let sizeLeft = download.totalBytes - download.currentBytes;
          summary.rawTimeLeft = Math.max(summary.rawTimeLeft,
                                         sizeLeft / download.speed);
          summary.slowestSpeed = Math.min(summary.slowestSpeed,
                                          download.speed);
        }
      } else if (download.canceled && download.hasPartialData) {
        summary.numPaused++;
      }

      
      if (download.succeeded) {
        summary.totalSize += download.target.size;
        summary.totalTransferred += download.target.size;
      } else if (download.hasProgress) {
        summary.totalSize += download.totalBytes;
        summary.totalTransferred += download.currentBytes;
      }
    }

    if (summary.totalSize != 0) {
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
                      (Ci.nsIPrompt.BUTTON_TITLE_IS_STRING * Ci.nsIPrompt.BUTTON_POS_1) +
                      Ci.nsIPrompt.BUTTON_POS_1_DEFAULT;
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
                                       okButton, cancelButton, null, null, {});
    return (rv == 0);
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

  
  this.oldDownloadStates = new Map();

  
  
  this._views = [];
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

  



  get downloads() this.oldDownloadStates.keys(),

  


  get canRemoveFinished() {
    for (let download of this.downloads) {
      
      if (download.stopped && !(download.canceled && download.hasPartialData)) {
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
    let indicatorData = this._isPrivate ? PrivateDownloadsIndicatorData
                                        : DownloadsIndicatorData;
    indicatorData.attention = false;
  },

  
  

  onDownloadAdded(download) {
    
    
    
    
    download.endTime = Date.now();

    this.oldDownloadStates.set(download,
                               DownloadsCommon.stateOfDownload(download));

    for (let view of this._views) {
      view.onDownloadAdded(download, true);
    }
  },

  onDownloadChanged(download) {
    let oldState = this.oldDownloadStates.get(download);
    let newState = DownloadsCommon.stateOfDownload(download);
    this.oldDownloadStates.set(download, newState);

    if (oldState != newState) {
      if (download.succeeded ||
          (download.canceled && !download.hasPartialData) ||
          download.error) {
        
        download.endTime = Date.now();

        
        
        
        
        if (!this._isPrivate) {
          try {
            let downloadMetaData = {
              state: DownloadsCommon.stateOfDownload(download),
              endTime: download.endTime,
            };
            if (download.succeeded) {
              downloadMetaData.fileSize = download.target.size;
            }
  
            PlacesUtils.annotations.setPageAnnotation(
                          NetUtil.newURI(download.source.url),
                          "downloads/metaData",
                          JSON.stringify(downloadMetaData), 0,
                          PlacesUtils.annotations.EXPIRE_WITH_HISTORY);
          } catch (ex) {
            Cu.reportError(ex);
          }
        }
      }

      for (let view of this._views) {
        try {
          view.onDownloadStateChanged(download);
        } catch (ex) {
          Cu.reportError(ex);
        }
      }

      if (download.succeeded ||
          (download.error && download.error.becauseBlocked)) {
        this._notifyDownloadEvent("finish");
      }
    }

    if (!download.newDownloadNotified) {
      download.newDownloadNotified = true;
      this._notifyDownloadEvent("start");
    }

    for (let view of this._views) {
      view.onDownloadChanged(download);
    }
  },

  onDownloadRemoved(download) {
    this.oldDownloadStates.delete(download);

    for (let view of this._views) {
      view.onDownloadRemoved(download);
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

    
    
    let downloadsArray = [...this.downloads];
    downloadsArray.sort((a, b) => b.startTime - a.startTime);
    downloadsArray.forEach(download => aView.onDownloadAdded(download, false));

    
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

  














  onDownloadAdded(download, newest) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  








  onDownloadStateChanged(download) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  








  onDownloadChanged(download) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  








  onDownloadRemoved(download) {
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

  onDownloadAdded(download, newest) {
    this._itemCount++;
    this._updateViews();
  },

  onDownloadStateChanged(download) {
    if (download.succeeded || download.error) {
      this.attention = true;
    }

    
    this._lastRawTimeLeft = -1;
    this._lastTimeLeft = -1;
  },

  onDownloadChanged(download) {
    this._updateViews();
  },

  onDownloadRemoved(download) {
    this._itemCount--;
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

  





  * _activeDownloads() {
    let downloads = this._isPrivate ? PrivateDownloadsData.downloads
                                    : DownloadsData.downloads;
    for (let download of downloads) {
      if (!download.stopped || (download.canceled && download.hasPartialData)) {
        yield download;
      }
    }
  },

  


  _refreshProperties() {
    let summary =
      DownloadsCommon.summarizeDownloads(this._activeDownloads());

    
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

  this._downloads = [];

  
  
  
  
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
      
      
      this._downloads = [];
    }
  },

  
  
  
  

  onDataLoadCompleted() {
    DownloadsViewPrototype.onDataLoadCompleted.call(this);
    this._updateViews();
  },

  onDownloadAdded(download, newest) {
    if (newest) {
      this._downloads.unshift(download);
    } else {
      this._downloads.push(download);
    }

    this._updateViews();
  },

  onDownloadStateChanged() {
    
    this._lastRawTimeLeft = -1;
    this._lastTimeLeft = -1;
  },

  onDownloadChanged() {
    this._updateViews();
  },

  onDownloadRemoved(download) {
    let itemIndex = this._downloads.indexOf(download);
    this._downloads.splice(itemIndex, 1);
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

  
  

  






  * _downloadsForSummary() {
    if (this._downloads.length > 0) {
      for (let i = this._numToExclude; i < this._downloads.length; ++i) {
        yield this._downloads[i];
      }
    }
  },

  


  _refreshProperties() {
    
    let summary =
      DownloadsCommon.summarizeDownloads(this._downloadsForSummary());

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
