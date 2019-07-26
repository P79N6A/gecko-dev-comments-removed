




const URI_GENERIC_ICON_DOWNLOAD = "chrome://browser/skin/images/alert-downloads-30.png";

var Downloads = {
  _inited: false,
  _progressAlert: null,

  




  _downloadCount: 0,
  _lastSec: Infinity,
  _notificationBox: null,
  _progressNotification: null,
  _progressNotificationInfo: new Map(),
  _runDownloadBooleanMap: new Map(),

  get manager() {
    return Cc["@mozilla.org/download-manager;1"]
             .getService(Ci.nsIDownloadManager);
  },

  _getReferrerOrSource: function dh__getReferrerOrSource(aDownload) {
    return aDownload.referrer.spec || aDownload.source.spec;
  },

  _getLocalFile: function dh__getLocalFile(aFileURI) {
    
    let spec = ('string' == typeof aFileURI) ? aFileURI : aFileURI.spec;
    let fileUrl;
    try {
      fileUrl = Services.io.newURI(spec, null, null).QueryInterface(Ci.nsIFileURL);
    } catch (ex) {
      Util.dumpLn("_getLocalFile: Caught exception creating newURI from file spec: "+aFileURI.spec+": " + ex.message);
      return;
    }
    return fileUrl.file.clone().QueryInterface(Ci.nsILocalFile);
  },

  init: function dh_init() {
    if (this._inited)
      return;

    this._inited = true;

    Services.obs.addObserver(this, "dl-start", true);
    Services.obs.addObserver(this, "dl-done", true);
    Services.obs.addObserver(this, "dl-run", true);
    Services.obs.addObserver(this, "dl-failed", true);

    this._notificationBox = Browser.getNotificationBox();
  },

  uninit: function dh_uninit() {
    if (this._inited) {
      Services.obs.removeObserver(this, "dl-start");
      Services.obs.removeObserver(this, "dl-done");
      Services.obs.removeObserver(this, "dl-run");
      Services.obs.removeObserver(this, "dl-failed");
    }
  },

  openDownload: function dh_openDownload(aDownload) {
    let fileURI = aDownload.target

    if (!(fileURI && fileURI.spec)) {
      Util.dumpLn("Cant open download "+id+", fileURI is invalid");
      return;
    }

    let file = this._getLocalFile(fileURI);
    try {
      file && MetroUtils.launchInDesktop(aDownload.target.spec, "");
    } catch (ex) {
      Util.dumpLn("Failed to open download, with id: "+id+", download target URI spec: " + fileURI.spec);
      Util.dumpLn("Failed download source:"+(aDownload.source && aDownload.source.spec));
    }
  },

  removeDownload: function dh_removeDownload(aDownload) {
    
    
    let id = aDownload.getAttribute("downloadId");
    let download = this.manager.getDownload(id);

    if (download) {
      this.manager.removeDownload(id);
    }
  },

  cancelDownload: function dh_cancelDownload(aDownload) {
    this._progressNotificationInfo.delete(aDownload.guid);
    this._runDownloadBooleanMap.delete(aDownload.targetFile.path);
    this._downloadCount--;
    if (this._progressNotificationInfo.size == 0) {
      this._notificationBox.removeNotification(this._progressNotification);
      this._progressNotification = null;
    }

    let fileURI = aDownload.target;
    if (!(fileURI && fileURI.spec)) {
      Util.dumpLn("Cant remove download file for: "+aDownload.id+", fileURI is invalid");
      return;
    }

    let file = this._getLocalFile(fileURI);
    try {
      this.manager.cancelDownload(aDownload.id);
      if (file && file.exists())
        file.remove(false);
    } catch (ex) {
      Util.dumpLn("Failed to cancel download, with id: "+aDownload.id+", download target URI spec: " + fileURI.spec);
      Util.dumpLn("Failed download source:"+(aDownload.source && aDownload.source.spec));
    }
  },

  
  cancelDownloads: function dh_cancelDownloads() {
    for (let info of this._progressNotificationInfo) {
      this.cancelDownload(info[1].download);
    }
    this._downloadCount = 0;
    this._progressNotificationInfo.clear();
    this._runDownloadBooleanMap.clear();
  },

  pauseDownload: function dh_pauseDownload(aDownload) {
    let id = aDownload.getAttribute("downloadId");
    this.manager.pauseDownload(id);
  },

  resumeDownload: function dh_resumeDownload(aDownload) {
    let id = aDownload.getAttribute("downloadId");
    this.manager.resumeDownload(id);
  },

  showPage: function dh_showPage(aDownload) {
    let id = aDownload.getAttribute("downloadId");
    let download = this.manager.getDownload(id);
    let uri = this._getReferrerOrSource(download);
    if (uri)
      BrowserUI.newTab(uri, Browser.selectedTab);
  },

  showAlert: function dh_showAlert(aName, aMessage, aTitle, aIcon) {
    var notifier = Cc["@mozilla.org/alerts-service;1"]
                     .getService(Ci.nsIAlertsService);

    
    let observer = {
      observe: function (aSubject, aTopic, aData) {
        if (aTopic == "alertclickcallback")
          PanelUI.show("downloads-container");
      }
    };

    if (!aTitle)
      aTitle = Strings.browser.GetStringFromName("alertDownloads");

    if (!aIcon)
      aIcon = URI_GENERIC_ICON_DOWNLOAD;

    notifier.showAlertNotification(aIcon, aTitle, aMessage, true, "", observer, aName);
  },

  showNotification: function dh_showNotification(title, msg, buttons, priority) {
    return this._notificationBox.appendNotification(msg,
                                              title,
                                              URI_GENERIC_ICON_DOWNLOAD,
                                              priority,
                                              buttons);
  },

  _showDownloadFailedNotification: function (aDownload) {
    let tryAgainButtonText =
      Strings.browser.GetStringFromName("downloadTryAgain");
    let cancelButtonText =
      Strings.browser.GetStringFromName("downloadCancel");

    let message = Strings.browser.formatStringFromName("alertDownloadFailed",
      [aDownload.displayName], 1);

    let buttons = [
      {
        isDefault: true,
        label: tryAgainButtonText,
        accessKey: "",
        callback: function() {
          Downloads.manager.retryDownload(aDownload.id);
        }
      },
      {
        label: cancelButtonText,
        accessKey: "",
        callback: function() {
          Downloads.cancelDownload(aDownload);
        }
      }
    ];
    this.showNotification("download-failed", message, buttons,
      this._notificationBox.PRIORITY_WARNING_HIGH);
  },

  _showDownloadCompleteNotification: function (aDownload) {
    let message = "";
    let showInFilesButtonText = PluralForm.get(this._downloadCount,
      Strings.browser.GetStringFromName("downloadsShowInFiles"));

    let buttons = [
      {
        label: showInFilesButtonText,
        accessKey: "",
        callback: function() {
          let fileURI = aDownload.target;
          let file = Downloads._getLocalFile(fileURI);
          file.reveal();
        }
      }
    ];

    if (this._downloadCount > 1) {
      message = PluralForm.get(this._downloadCount,
                               Strings.browser.GetStringFromName("alertMultipleDownloadsComplete"))
                               .replace("#1", this._downloadCount)
    } else {
      let runButtonText =
        Strings.browser.GetStringFromName("downloadRun");
      message = Strings.browser.formatStringFromName("alertDownloadsDone2",
        [aDownload.displayName], 1);

      buttons.unshift({
        isDefault: true,
        label: runButtonText,
        accessKey: "",
        callback: function() {
          Downloads.openDownload(aDownload);
        }
      });
    }
    this.showNotification("download-complete", message, buttons,
      this._notificationBox.PRIORITY_WARNING_MEDIUM);
  },

  _computeDownloadProgressString: function dv_computeDownloadProgressString(aDownload) {
    let totTransferred = 0, totSize = 0, totSecondsLeft = 0;
    for (let info of this._progressNotificationInfo) {
      let size = info[1].download.size;
      let amountTransferred = info[1].download.amountTransferred;
      let speed = info[1].download.speed;

      totTransferred += amountTransferred;
      totSize += size;
      totSecondsLeft += ((size - amountTransferred) / speed);
    }
    
    let amountTransferred = Util.getDownloadSize(totTransferred);
    let size = Util.getDownloadSize(totSize);
    let progress = amountTransferred + "/" + size;

    
    let [timeLeft, newLast] = DownloadUtils.getTimeLeft(totSecondsLeft, this._lastSec);
    this._lastSec = newLast;

    if (this._downloadCount == 1) {
      return Strings.browser.GetStringFromName("alertDownloadsStart2")
        .replace("#1", aDownload.displayName)
        .replace("#2", progress)
        .replace("#3", timeLeft)
    }

    let numDownloads = this._downloadCount;
    return PluralForm.get(numDownloads,
                          Strings.browser.GetStringFromName("alertDownloadMultiple"))
                          .replace("#1", numDownloads)
                          .replace("#2", progress)
                          .replace("#3", timeLeft);
  },

  _saveDownloadData: function dv_saveDownloadData(aDownload) {
    if (!this._progressNotificationInfo.get(aDownload.guid)) {
      this._progressNotificationInfo.set(aDownload.guid, {});
    }
    let infoObj = this._progressNotificationInfo.get(aDownload.guid);
    infoObj.download = aDownload;
    this._progressNotificationInfo.set(aDownload.guid, infoObj);
  },

  updateInfobar: function dv_updateInfobar(aDownload) {
    this._saveDownloadData(aDownload);
    let message = this._computeDownloadProgressString(aDownload);

    if (this._progressNotification == null ||
        !this._notificationBox.getNotificationWithValue("download-progress")) {

      let cancelButtonText =
              Strings.browser.GetStringFromName("downloadCancel");

      let buttons = [
        {
          isDefault: false,
          label: cancelButtonText,
          accessKey: "",
          callback: function() {
            Downloads.cancelDownloads();
          }
        }
      ];

      this._progressNotification =
        this.showNotification("download-progress", message, buttons,
        this._notificationBox.PRIORITY_WARNING_LOW);
    } else {
      this._progressNotification.label = message;
    }
  },

  observe: function (aSubject, aTopic, aData) {
    let message = "";
    let msgTitle = "";

    switch (aTopic) {
      case "dl-run":
        let file = aSubject.QueryInterface(Ci.nsIFile);
        this._runDownloadBooleanMap.set(file.path, (aData == 'true'));
        break;
      case "dl-start":
        this._downloadCount++;
        let download = aSubject.QueryInterface(Ci.nsIDownload);
        if (!this._progressNotificationInfo.get(download.guid)) {
          this._progressNotificationInfo.set(download.guid, {});
        }
        if (!this._progressAlert) {
          this._progressAlert = new AlertDownloadProgressListener();
          this.manager.addListener(this._progressAlert);
        }
        this.updateInfobar(download);
        break;
      case "dl-done":
        download = aSubject.QueryInterface(Ci.nsIDownload);
        let runAfterDownload = this._runDownloadBooleanMap.get(download.targetFile.path);
        if (runAfterDownload) {
          this.openDownload(download);
        }

        this._progressNotificationInfo.delete(download.guid);
        this._runDownloadBooleanMap.delete(download.targetFile.path);
        if (this._progressNotificationInfo.size == 0) {
          if (this._downloadCount > 1 || !runAfterDownload) {
            this._showDownloadCompleteNotification(download);
          }
          this._downloadCount = 0;
          this._notificationBox.removeNotification(this._progressNotification);
          this._progressNotification = null;
        }
        break;
      case "dl-failed":
        download = aSubject.QueryInterface(Ci.nsIDownload);
        break;
    }
  },

  QueryInterface: function (aIID) {
    if (!aIID.equals(Ci.nsIObserver) &&
        !aIID.equals(Ci.nsISupportsWeakReference) &&
        !aIID.equals(Ci.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};









function DownloadsView(aSet, aLimit) {
  this._set = aSet;
  this._limit = aLimit;

  this._progress = new DownloadProgressListener(this);
  Downloads.manager.addListener(this._progress);

  
  let obs = Cc["@mozilla.org/observer-service;1"].
                  getService(Ci.nsIObserverService);
  obs.addObserver(this, "download-manager-remove-download-guid", false);

  this.getDownloads();
}

DownloadsView.prototype = {
  _progress: null,
  _stmt: null,
  _timeoutID: null,
  _set: null,
  _limit: null,

  _getItemForDownloadGuid: function dv__getItemForDownload(aGuid) {
    return this._set.querySelector("richgriditem[downloadGuid='" + aGuid + "']");
  },

  _getItemForDownload: function dv__getItemForDownload(aDownload) {
    return this._set.querySelector("richgriditem[downloadId='" + aDownload.id + "']");
  },

  _getDownloadForItem: function dv__getDownloadForItem(anItem) {
    let id = anItem.getAttribute("downloadId");
    return Downloads.manager.getDownload(id);
  },

  _getAttrsForDownload: function dv__getAttrsForDownload(aDownload) {
    
    return {
      typeName: 'download',
      downloadId: aDownload.id,
      downloadGuid: aDownload.guid,
      name: aDownload.displayName,
      
      target: aDownload.target.spec,
      iconURI: "moz-icon://" + aDownload.displayName + "?size=64",
      date: DownloadUtils.getReadableDates(new Date())[0],
      domain: DownloadUtils.getURIHost(aDownload.source.spec)[0],
      size: Util.getDownloadSize(aDownload.size),
      state: aDownload.state
    };

  },
  _updateItemWithAttrs: function dv__updateItemWithAttrs(anItem, aAttrs) {
    for (let name in aAttrs)
      anItem.setAttribute(name, aAttrs[name]);
    if (anItem.refresh)
      anItem.refresh();
  },

  _initStatement: function dv__initStatement() {
    if (this._stmt)
      this._stmt.finalize();

    let limitClause = this._limit ? ("LIMIT " + this._limit) : "";

    this._stmt = Downloads.manager.DBConnection.createStatement(
      "SELECT id, guid, name, target, source, state, startTime, endTime, referrer, " +
             "currBytes, maxBytes, state IN (?1, ?2, ?3, ?4, ?5) isActive " +
      "FROM moz_downloads " +
      "ORDER BY isActive DESC, endTime DESC, startTime DESC " +
      limitClause);
  },

  _stepDownloads: function dv__stepDownloads(aNumItems) {
    try {
      if (!this._stmt.executeStep()) {
        
        this._stmt.finalize();
        this._stmt = null;
        this._fire("DownloadsReady", this._set);
        return;
      }
      let attrs = {
        typeName: 'download',
        
        downloadGuid: this._stmt.row.guid,
        downloadId: this._stmt.row.id,
        name: this._stmt.row.name,
        target: this._stmt.row.target,
        iconURI: "moz-icon://" + this._stmt.row.name + "?size=25",
        date: DownloadUtils.getReadableDates(new Date(this._stmt.row.endTime / 1000))[0],
        domain: DownloadUtils.getURIHost(this._stmt.row.source)[0],
        size: Util.getDownloadSize(this._stmt.row.maxBytes),
        state: this._stmt.row.state
      };

      let item = this._set.appendItem(attrs.target, attrs.downloadId);
      this._updateItemWithAttrs(item, attrs);
    } catch (e) {
      
      this._stmt.reset();
      return;
    }

    
    
    if (aNumItems > 1) {
      this._stepDownloads(aNumItems - 1);
    } else {
      
      let delay = Math.min(this._set.itemCount * 10, 300);
      let self = this;
      this._timeoutID = setTimeout(function() { self._stepDownloads(5); }, delay);
    }
  },

  _fire: function _fire(aName, anElement) {
    let event = document.createEvent("Events");
    event.initEvent(aName, true, true);
    anElement.dispatchEvent(event);
  },

  observe: function dv_managerObserver(aSubject, aTopic, aData) {
    
    switch (aTopic) {
      case "download-manager-remove-download-guid":
        let guid = aSubject.QueryInterface(Ci.nsISupportsCString);
        this.removeDownload({
          guid: guid
        });
        return;
    }
  },

  getDownloads: function dv_getDownloads() {
    this._initStatement();
    clearTimeout(this._timeoutID);

    
    this.clearDownloads();

    this._stmt.reset();
    this._stmt.bindInt32Parameter(0, Ci.nsIDownloadManager.DOWNLOAD_NOTSTARTED);
    this._stmt.bindInt32Parameter(1, Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING);
    this._stmt.bindInt32Parameter(2, Ci.nsIDownloadManager.DOWNLOAD_PAUSED);
    this._stmt.bindInt32Parameter(3, Ci.nsIDownloadManager.DOWNLOAD_QUEUED);
    this._stmt.bindInt32Parameter(4, Ci.nsIDownloadManager.DOWNLOAD_SCANNING);

    
    let self = this;
    this._timeoutID = setTimeout(function() {
      self._stepDownloads(1);
    }, 0);
  },

  clearDownloads: function dv_clearDownloads() {
    this._set.clearAll();
  },

  addDownload: function dv_addDownload(aDownload) {
    
    let attrs = this._getAttrsForDownload(aDownload);
    let item = this._set.insertItemAt(0, attrs.target, attrs.downloadId);
    this._updateItemWithAttrs(item, attrs);
  },

  updateDownload: function dv_updateDownload(aDownload) {
    
    let item = this._getItemForDownload(aDownload);

    if (!item)
      return;

    let attrs = this._getAttrsForDownload(aDownload);
    this._updateItemWithAttrs(item, attrs);

    if (Downloads._progressNotification != null) {
      Downloads._saveDownloadData(aDownload);
      Downloads._progressNotification.label =
        Downloads._computeDownloadProgressString(aDownload);
    }
  },

  removeDownload: function dv_removeDownload(aDownload) {
    
    let item;
    if (aDownload.id) {
      item = this._getItemForDownload(aDownload.id);
    } else if (aDownload.guid) {
      item = this._getItemForDownloadGuid(aDownload.guid);
    }
    if (!item)
      return;

    let idx = this._set.getIndexOfItem(item);
    if (idx < 0)
        return;
    
    this._set.removeItemAt(idx);
  },

  destruct: function dv_destruct() {
    Downloads.manager.removeListener(this._progress);
  }
};

var DownloadsPanelView = {
  _view: null,

  get _grid() { return document.getElementById("downloads-list"); },
  get visible() { return PanelUI.isPaneVisible("downloads-container"); },

  init: function init() {
    this._view = new DownloadsView(this._grid);
  },

  show: function show() {
    this._grid.arrangeItems();
  },

  uninit: function uninit() {
    this._view.destruct();
  }
};






function DownloadProgressListener(aView) {
  this._view = aView;
}

DownloadProgressListener.prototype = {
  _view: null,

  
  
  onDownloadStateChange: function dPL_onDownloadStateChange(aState, aDownload) {
    let state = aDownload.state;
    switch (state) {
      case Ci.nsIDownloadManager.DOWNLOAD_QUEUED:
      case Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_POLICY:
        this._view.addDownload(aDownload);
        break;
      case Ci.nsIDownloadManager.DOWNLOAD_FAILED:
      case Ci.nsIDownloadManager.DOWNLOAD_CANCELED:
      case Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_PARENTAL:
      case Ci.nsIDownloadManager.DOWNLOAD_DIRTY:
      case Ci.nsIDownloadManager.DOWNLOAD_FINISHED:
        break;
    }

    this._view.updateDownload(aDownload);
  },

  onProgressChange: function dPL_onProgressChange(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress, aDownload) {
    
    this._view.updateDownload(aDownload);
  },

  onStateChange: function(aWebProgress, aRequest, aState, aStatus, aDownload) { },
  onSecurityChange: function(aWebProgress, aRequest, aState, aDownload) { },

  
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDownloadProgressListener])
};






function AlertDownloadProgressListener() { }

AlertDownloadProgressListener.prototype = {
  
  
  onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress, aDownload) {
    let strings = Strings.browser;
    let availableSpace = -1;

    try {
      
      let availableSpace = aDownload.targetFile.diskSpaceAvailable;
    } catch(ex) { }

    let contentLength = aDownload.size;
    if (availableSpace > 0 && contentLength > 0 && contentLength > availableSpace) {
      Downloads.showAlert(aDownload.target.spec.replace("file:", "download:"),
                          strings.GetStringFromName("alertDownloadsNoSpace"),
                          strings.GetStringFromName("alertDownloadsSize"));
      Downloads.cancelDownload(aDownload);
    }
  },

  onDownloadStateChange: function(aState, aDownload) { },
  onStateChange: function(aWebProgress, aRequest, aState, aStatus, aDownload) { },
  onSecurityChange: function(aWebProgress, aRequest, aState, aDownload) { },

  
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDownloadProgressListener])
};
