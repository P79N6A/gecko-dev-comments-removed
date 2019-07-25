





"use strict";

var EXPORTED_SYMBOLS = [
  "DownloadsCommon",
];






























const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gBrowserGlue",
                                   "@mozilla.org/browser/browserglue;1",
                                   "nsIBrowserGlue");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");

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
  showMoreDownloads: true
};

XPCOMUtils.defineLazyGetter(this, "DownloadsLocalFileCtor", function () {
  return Components.Constructor("@mozilla.org/file/local;1",
                                "nsILocalFile", "initWithPath");
});








const DownloadsCommon = {
  




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

  





  get data() DownloadsData,

  





  get indicatorData() DownloadsIndicatorData
};




XPCOMUtils.defineLazyGetter(DownloadsCommon, "isWinVistaOrHigher", function () {
  let os = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
  if (os != "WINNT") {
    return false;
  }
  let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
  return parseFloat(sysInfo.getProperty("version")) >= 6;
});



















const DownloadsData = {
  








  initializeDataLink: function DD_initializeDataLink(aDownloadManagerService)
  {
    
    aDownloadManagerService.addListener(this);
    Services.obs.addObserver(this, "download-manager-remove-download", false);
    Services.obs.addObserver(this, "download-manager-database-type-changed",
                             false);
  },

  


  terminateDataLink: function DD_terminateDataLink()
  {
    this._terminateDataAccess();

    
    Services.obs.removeObserver(this, "download-manager-database-type-changed");
    Services.obs.removeObserver(this, "download-manager-remove-download");
    Services.downloads.removeListener(this);
  },

  
  

  



  _views: [],

  







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
    loadedItemsArray.sort(function(a, b) b.downloadId - a.downloadId);
    loadedItemsArray.forEach(
      function (dataItem) aView.onDataItemAdded(dataItem, false)
    );

    
    if (!this._pendingStatement) {
      aView.onDataLoadCompleted();
    }
  },

  
  

  






  dataItems: {},

  



  _persistentDataItems: {},

  


  clear: function DD_clear()
  {
    this._terminateDataAccess();
    this.dataItems = {};
  },

  


























  _getOrAddDataItem: function DD_getOrAddDataItem(aSource, aMayReuseId)
  {
    let downloadId = (aSource instanceof Ci.nsIDownload)
                     ? aSource.id
                     : aSource.getResultByName("id");
    if (downloadId in this.dataItems) {
      let existingItem = this.dataItems[downloadId];
      if (existingItem || !aMayReuseId) {
        
        return existingItem;
      }
    }

    let dataItem = new DownloadsDataItem(aSource);
    this.dataItems[downloadId] = dataItem;

    
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
      this._views.forEach(
        function (view) view.onDataItemRemoved(dataItem)
      );
    }
    this.dataItems[aDownloadId] = null;
  },

  
  

  


  _pendingStatement: null,

  




  _loadState: 0,

  
  get kLoadNone() 0,
  
  get kLoadActive() 1,
  
  get kLoadAll() 2,

  






  ensurePersistentDataLoaded:
  function DD_ensurePersistentDataLoaded(aActiveOnly)
  {
    if (this._pendingStatement) {
      
      return;
    }

    if (aActiveOnly) {
      if (this._loadState == this.kLoadNone) {
        
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
      }
    } else {
      if (this._loadState != this.kLoadAll) {
        
        
        
        
        let statement = Services.downloads.DBConnection.createAsyncStatement(
          "SELECT id, target, name, source, referrer, state, "
        +        "startTime, endTime, currBytes, maxBytes "
        + "FROM moz_downloads "
        + "ORDER BY id DESC"
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
    Cu.reportError("Database statement execution error (" + aError.result +
                   "): " + aError.message);
  },

  handleCompletion: function DD_handleCompletion(aReason)
  {
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
      case "download-manager-remove-download":
        
        if (aSubject) {
          this._removeDataItem(aSubject.QueryInterface(Ci.nsISupportsPRUint32));
          break;
        }

        
        
        for each (let dataItem in this.dataItems) {
          if (dataItem) {
            try {
              Services.downloads.getDownload(dataItem.downloadId);
            } catch (ex) {
              this._removeDataItem(dataItem.downloadId);
            }
          }
        }
        break;

      case "download-manager-database-type-changed":
        let pbs = Cc["@mozilla.org/privatebrowsing;1"]
                  .getService(Ci.nsIPrivateBrowsingService);
        if (pbs.privateBrowsingEnabled) {
          
          this._persistentDataItems = this.dataItems;
          this.clear();
        } else {
          
          this.clear();
          this.dataItems = this._persistentDataItems;
          this._persistentDataItems = null;
        }
        
        
        this._views.forEach(this._updateView, this);
        break;
    }
  },

  
  

  onDownloadStateChange: function DD_onDownloadStateChange(aState, aDownload)
  {
    
    
    
    let isNew = aState == nsIDM.DOWNLOAD_NOTSTARTED ||
                aState == nsIDM.DOWNLOAD_QUEUED;

    let dataItem = this._getOrAddDataItem(aDownload, isNew);
    if (!dataItem) {
      return;
    }

    dataItem.state = aDownload.state;
    dataItem.referrer = aDownload.referrer && aDownload.referrer.spec;
    dataItem.resumable = aDownload.resumable;
    dataItem.startTime = Math.round(aDownload.startTime / 1000);
    dataItem.currBytes = aDownload.amountTransferred;
    dataItem.maxBytes = aDownload.size;

    this._views.forEach(
      function (view) view.getViewItem(dataItem).onStateChange()
    );

    if (isNew && !dataItem.newDownloadNotified) {
      dataItem.newDownloadNotified = true;
      this._notifyNewDownload();
    }
  },

  onProgressChange: function DD_onProgressChange(aWebProgress, aRequest,
                                                  aCurSelfProgress,
                                                  aMaxSelfProgress,
                                                  aCurTotalProgress,
                                                  aMaxTotalProgress, aDownload)
  {
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

  
  

  



  firstDownloadShown: false,

  



  _notifyNewDownload: function DD_notifyNewDownload()
  {
    if (DownloadsCommon.useToolkitUI) {
      return;
    }

    
    let browserWin = gBrowserGlue.getMostRecentBrowserWindow();
    if (!browserWin) {
      return;
    }

    browserWin.focus();
    if (this.firstDownloadShown) {
      
      
      
      browserWin.DownloadsIndicatorView.showEventNotification();
      return;
    }
    this.firstDownloadShown = true;
    browserWin.DownloadsPanel.showPanel();
  }
};














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
    this.download = aDownload;

    
    this.downloadId = aDownload.id;
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
    
    this.downloadId = aStorageRow.getResultByName("id");
    this.file = aStorageRow.getResultByName("target");
    this.target = aStorageRow.getResultByName("name");
    this.uri = aStorageRow.getResultByName("source");
    this.referrer = aStorageRow.getResultByName("referrer");
    this.state = aStorageRow.getResultByName("state");
    this.startTime = Math.round(aStorageRow.getResultByName("startTime") / 1000);
    this.endTime = Math.round(aStorageRow.getResultByName("endTime") / 1000);
    this.currBytes = aStorageRow.getResultByName("currBytes");
    this.maxBytes = aStorageRow.getResultByName("maxBytes");

    
    XPCOMUtils.defineLazyGetter(this, "download", function ()
                                Services.downloads.getDownload(this.downloadId));

    
    
    
    
    
    
    
    if (this.state == nsIDM.DOWNLOAD_DOWNLOADING) {
      this.resumable = this.download.resumable;
    } else {
      this.resumable = true;
    }

    
    this.speed = 0;
    this.percentComplete = this.maxBytes <= 0
                           ? -1
                           : Math.round(this.currBytes / this.maxBytes * 100);
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
    
    
    
    if (/^file:/.test(this.file)) {
      
      
      let fileUrl = NetUtil.newURI(this.file).QueryInterface(Ci.nsIFileURL);
      return fileUrl.file.clone().QueryInterface(Ci.nsILocalFile);
    } else {
      
      
      return new DownloadsLocalFileCtor(this.file);
    }
  }
};














const DownloadsIndicatorData = {
  
  

  



  _views: [],

  







  addView: function DID_addView(aView)
  {
    
    if (this._views.length == 0) {
      DownloadsCommon.data.addView(this);
    }

    this._views.push(aView);
    this.refreshView(aView);
  },

  





  refreshView: function DID_refreshView(aView)
  {
    
    this._refreshProperties();
    this._updateView(aView);
  },

  





  removeView: function DID_removeView(aView)
  {
    let index = this._views.indexOf(aView);
    if (index != -1) {
      this._views.splice(index, 1);
    }

    
    if (this._views.length == 0) {
      DownloadsCommon.data.removeView(this);
      this._itemCount = 0;
    }
  },

  
  

  


  _loading: false,

  


  onDataLoadStarting: function DID_onDataLoadStarting()
  {
    this._loading = true;
  },

  


  onDataLoadCompleted: function DID_onDataLoadCompleted()
  {
    this._loading = false;
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
    return Object.freeze({
      onStateChange: function DIVI_onStateChange()
      {
        if (aDataItem.state == nsIDM.DOWNLOAD_FINISHED ||
            aDataItem.state == nsIDM.DOWNLOAD_FAILED) {
          DownloadsIndicatorData.attention = true;
        }

        
        DownloadsIndicatorData._lastRawTimeLeft = -1;
        DownloadsIndicatorData._lastTimeLeft = -1;

        DownloadsIndicatorData._updateViews();
      },
      onProgressChange: function DIVI_onProgressChange()
      {
        DownloadsIndicatorData._updateViews();
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

  







  _updateTimeLeft: function DID_updateTimeLeft(aSeconds)
  {
    
    
    
    let shouldApplySmoothing = this._lastTimeLeft >= 0 &&
                               aSeconds > this._lastTimeLeft / 2;
    if (shouldApplySmoothing) {
      
      
      let (diff = aSeconds - this._lastTimeLeft) {
        aSeconds = this._lastTimeLeft + (diff < 0 ? .3 : .1) * diff;
      }

      
      
      let diff = aSeconds - this._lastTimeLeft;
      let diffPercent = diff / this._lastTimeLeft * 100;
      if (Math.abs(diff) < 5 || Math.abs(diffPercent) < 5) {
        aSeconds = this._lastTimeLeft - (diff < 0 ? .4 : .2);
      }
    }

    
    
    
    this._lastTimeLeft = Math.max(aSeconds, 1);
  },

  


  _refreshProperties: function DID_refreshProperties()
  {
    let numActive = 0;
    let numPaused = 0;
    let numScanning = 0;
    let totalSize = 0;
    let totalTransferred = 0;
    let rawTimeLeft = -1;

    
    
    if (this._itemCount > 0) {
      let downloads = Services.downloads.activeDownloads;
      while (downloads.hasMoreElements()) {
        let download = downloads.getNext().QueryInterface(Ci.nsIDownload);
        numActive++;
        switch (download.state) {
          case nsIDM.DOWNLOAD_PAUSED:
            numPaused++;
            break;
          case nsIDM.DOWNLOAD_SCANNING:
            numScanning++;
            break;
          case nsIDM.DOWNLOAD_DOWNLOADING:
            if (download.size > 0 && download.speed > 0) {
              let sizeLeft = download.size - download.amountTransferred;
              rawTimeLeft = Math.max(rawTimeLeft, sizeLeft / download.speed);
            }
            break;
        }
        
        if (download.size > 0) {
          totalSize += download.size;
          totalTransferred += download.amountTransferred;
        }
      }
    }

    
    this._hasDownloads = (this._itemCount > 0);

    if (numActive == 0 || totalSize == 0 || numActive == numScanning) {
      
      this._percentComplete = -1;
    } else {
      
      this._percentComplete = (totalTransferred / totalSize) * 100;
    }

    
    this._paused = numActive > 0 && numActive == numPaused;

    
    if (rawTimeLeft == -1) {
      
      this._lastRawTimeLeft = -1;
      this._lastTimeLeft = -1;
      this._counter = "";
    } else {
      
      if (this._lastRawTimeLeft != rawTimeLeft) {
        this._lastRawTimeLeft = rawTimeLeft;
        this._updateTimeLeft(rawTimeLeft);
      }
      this._counter = DownloadsCommon.formatTimeLeft(this._lastTimeLeft);
    }
  }
}
