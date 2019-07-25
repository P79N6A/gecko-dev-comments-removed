




































Components.utils.import("resource://gre/modules/DownloadUtils.jsm");

#ifdef ANDROID
const URI_GENERIC_ICON_DOWNLOAD = "drawable://alertdownloads";
#else
const URI_GENERIC_ICON_DOWNLOAD = "chrome://browser/skin/images/alert-downloads-30.png";
#endif

var DownloadsView = {
  _initialized: false,
  _list: null,
  _dlmgr: null,
  _progress: null,
  _progressAlert: null,

  _initStatement: function dv__initStatement() {
    if (this._stmt)
      this._stmt.finalize();

    this._stmt = this._dlmgr.DBConnection.createStatement(
      "SELECT id, target, name, source, state, startTime, endTime, referrer, " +
             "currBytes, maxBytes, state IN (?1, ?2, ?3, ?4, ?5) isActive " +
      "FROM moz_downloads " +
      "ORDER BY isActive DESC, endTime DESC, startTime DESC");
  },

  _getLocalFile: function dv__getLocalFile(aFileURI) {
    
    
    const fileUrl = Services.io.newURI(aFileURI, null, null).QueryInterface(Ci.nsIFileURL);
    return fileUrl.file.clone().QueryInterface(Ci.nsILocalFile);
  },

  _getReferrerOrSource: function dv__getReferrerOrSource(aItem) {
    
    return aItem.getAttribute("referrer") || aItem.getAttribute("uri");
  },

  _createItem: function dv__createItem(aAttrs) {
    
    let item = this.getElementForDownload(aAttrs.id);
    if (!item) {
      item = document.createElement("richlistitem");

      
      for (let attr in aAttrs)
        item.setAttribute(attr, aAttrs[attr]);
  
      
      item.setAttribute("typeName", "download");
      item.setAttribute("id", "dl-" + aAttrs.id);
      item.setAttribute("downloadID", aAttrs.id);
      item.setAttribute("iconURL", "moz-icon://" + aAttrs.file + "?size=32");
      item.setAttribute("lastSeconds", Infinity);
  
      
      this._updateTime(item);
      this._updateStatus(item);
    }
    return item;
  },

  _removeItem: function dv__removeItem(aItem) {
    
    if (!aItem)
      return;

    let index = this._list.selectedIndex;
    this._list.removeChild(aItem);
    this._list.selectedIndex = Math.min(index, this._list.itemCount - 1);
  },

  _clearList: function dv__clearList() {
    
    let header = document.getElementById("downloads-list-header");
    while (header.nextSibling)
      this._list.removeChild(header.nextSibling);
  },
  
  _ifEmptyShowMessage: function dv__ifEmptyShowMessage() {
    
    if (this._list.itemCount == 0) {
      let emptyString = Strings.browser.GetStringFromName("downloadsEmpty");
      let emptyItem = this._list.appendItem(emptyString);
      emptyItem.id = "dl-empty-message";
    }
  },

  get visible() {
    let items = document.getElementById("panel-items");
    if (BrowserUI.isPanelVisible() && items.selectedPanel.id == "downloads-container")
      return true;
    return false;
  },

  init: function dv_init() {
    if (this._initialized)
      return;
    this._initialized = true;

    
    var os = Services.obs;
    os.addObserver(this, "dl-start", true);
    os.addObserver(this, "dl-failed", true);
    os.addObserver(this, "dl-done", true);
    os.addObserver(this, "dl-blocked", true);
    os.addObserver(this, "dl-dirty", true);
    os.addObserver(this, "dl-cancel", true);

    
    os.addObserver(this, "download-manager-remove-download", true);

    let self = this;
    let panels = document.getElementById("panel-items");
    panels.addEventListener("select",
                            function(aEvent) {
                              if (panels.selectedPanel.id == "downloads-container")
                                self._delayedInit();
                            },
                            false);
  },

  _delayedInit: function dv__delayedInit() {
    if (this._list)
      return;

    this._list = document.getElementById("downloads-list");

    if (this._dlmgr == null)
      this._dlmgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);

    this._progress = new DownloadProgressListener();
    this._dlmgr.addListener(this._progress);

    this._initStatement();
    this.getDownloads();
  },

  getDownloads: function dv_getDownloads() {
    clearTimeout(this._timeoutID);
    this._stmt.reset();

    
    this._clearList();

    this._stmt.bindInt32Parameter(0, Ci.nsIDownloadManager.DOWNLOAD_NOTSTARTED);
    this._stmt.bindInt32Parameter(1, Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING);
    this._stmt.bindInt32Parameter(2, Ci.nsIDownloadManager.DOWNLOAD_PAUSED);
    this._stmt.bindInt32Parameter(3, Ci.nsIDownloadManager.DOWNLOAD_QUEUED);
    this._stmt.bindInt32Parameter(4, Ci.nsIDownloadManager.DOWNLOAD_SCANNING);

    
    let self = this;
    this._timeoutID = setTimeout(function() {
      
      self._stepDownloads(1);
      self._list.selectedIndex = 0;
    }, 0);
  },

  _stepDownloads: function dv__stepDownloads(aNumItems) {
    try {
      
      if (!this._stmt.executeStep()) {
        
        this._ifEmptyShowMessage();
        
        
        setTimeout(function() {
          Services.obs.notifyObservers(window, "download-manager-ui-done", null);
        }, 0);
        return;
      }

      
      let attrs = {
        id: this._stmt.getInt64(0),
        file: this._stmt.getString(1),
        target: this._stmt.getString(2),
        uri: this._stmt.getString(3),
        state: this._stmt.getInt32(4),
        startTime: Math.round(this._stmt.getInt64(5) / 1000),
        endTime: Math.round(this._stmt.getInt64(6) / 1000),
        currBytes: this._stmt.getInt64(8),
        maxBytes: this._stmt.getInt64(9)
      };

      
      let (referrer = this._stmt.getString(7)) {
        if (referrer)
          attrs.referrer = referrer;
      };

      
      let isActive = this._stmt.getInt32(10);
      attrs.progress = isActive ? this._dlmgr.getDownload(attrs.id).percentComplete : 100;

      
      let item = this._createItem(attrs);
      this._list.appendChild(item);
    }
    catch (e) {
      
      this._stmt.reset();
      return;
    }

    
    
    if (aNumItems > 1) {
      this._stepDownloads(aNumItems - 1);
    }
    else {
      
      let delay = Math.min(this._list.itemCount * 10, 300);
      let self = this;
      this._timeoutID = setTimeout(function() { self._stepDownloads(5); }, delay);
    }
  },

  downloadStarted: function dv_downloadStarted(aDownload) {
    let attrs = {
      id: aDownload.id,
      file: aDownload.target.spec,
      target: aDownload.displayName,
      uri: aDownload.source.spec,
      state: aDownload.state,
      progress: aDownload.percentComplete,
      startTime: Math.round(aDownload.startTime / 1000),
      endTime: Date.now(),
      currBytes: aDownload.amountTransferred,
      maxBytes: aDownload.size
    };

    
    let emptyItem = document.getElementById("dl-empty-message");
    if (emptyItem)
      this._list.removeChild(emptyItem);
      
    
    let header = document.getElementById("downloads-list-header");
    let item = this._createItem(attrs);
    this._list.insertBefore(item, header.nextSibling);
  },

  downloadCompleted: function dv_downloadCompleted(aDownload) {
    
    let element = this.getElementForDownload(aDownload.id);

    
    let next = element.nextSibling;
    while (next && next.inProgress)
      next = next.nextSibling;

    
    this._list.insertBefore(element, next);
  },

  _updateStatus: function dv__updateStatus(aItem) {
    let strings = Strings.browser;

    let status = "";

    
    let fileSize = Number(aItem.getAttribute("maxBytes"));
    let sizeText = strings.GetStringFromName("downloadsUnknownSize");
    if (fileSize >= 0) {
      let [size, unit] = DownloadUtils.convertByteUnits(fileSize);
      sizeText = this._replaceInsert(strings.GetStringFromName("downloadsKnownSize"), 1, size);
      sizeText = this._replaceInsert(sizeText, 2, unit);
    }

    
    status = this._replaceInsert(strings.GetStringFromName("downloadsStatus"), 1, sizeText);

    
    let [displayHost, fullHost] = DownloadUtils.getURIHost(this._getReferrerOrSource(aItem));
    status = this._replaceInsert(status, 2, displayHost);
  
    
    let currstate = Number(aItem.getAttribute("state"));
    if (currstate == Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING) {
      let downloadSize = Number(aItem.getAttribute("currBytes"));
      let passedTime = (Date.now() - aItem.getAttribute("startTime"))/1000;
      let totalTime = (passedTime / downloadSize) * fileSize;
      let leftTime = totalTime - passedTime;
      let [time, lastTime] = DownloadUtils.getTimeLeft(leftTime);

      let stringTime = this._replaceInsert(strings.GetStringFromName("downloadsTime"), 1, time);
      status = status + " " + stringTime;
    } 
      
    aItem.setAttribute("status", status);
  },

  _updateTime: function dv__updateTime(aItem) {
    
    if (aItem.inProgress)
      return;

    let dts = Cc["@mozilla.org/intl/scriptabledateformat;1"].getService(Ci.nsIScriptableDateFormat);

    
    let now = new Date();
    let today = new Date(now.getFullYear(), now.getMonth(), now.getDate());

    
    let end = new Date(parseInt(aItem.getAttribute("endTime")));

    let strings = Strings.browser;

    
    let dateTime;
    if (end >= today) {
      
      dateTime = dts.FormatTime("", dts.timeFormatNoSeconds, end.getHours(), end.getMinutes(), 0);
    }
    else if (today - end < (24 * 60 * 60 * 1000)) {
      
      dateTime = strings.GetStringFromName("donwloadsYesterday");
    }
    else if (today - end < (6 * 24 * 60 * 60 * 1000)) {
      
      dateTime = end.toLocaleFormat("%A");
    }
    else {
      
      let month = end.toLocaleFormat("%B");
      
      let date = Number(end.toLocaleFormat("%d"));
      dateTime = this._replaceInsert(strings.GetStringFromName("downloadsMonthDate"), 1, month);
      dateTime = this._replaceInsert(dateTime, 2, date);
    }

    aItem.setAttribute("datetime", dateTime);
  },

  _replaceInsert: function dv__replaceInsert(aText, aIndex, aValue) {
    return aText.replace("#" + aIndex, aValue);
  },

  getElementForDownload: function dv_getElementFromDownload(aID) {
    return document.getElementById("dl-" + aID);
  },

  openDownload: function dv_openDownload(aItem) {
    let f = this._getLocalFile(aItem.getAttribute("file"));
    try {
      f.launch();
    } catch (ex) { }

    
  },

  removeDownload: function dv_removeDownload(aItem) {
    let f = this._getLocalFile(aItem.getAttribute("file"));
    let res = Services.prompt.confirm(null, Strings.browser.GetStringFromName("downloadsDeleteTitle"), f.leafName);
    if(res) {
      this._dlmgr.removeDownload(aItem.getAttribute("downloadID"));
      if (f.exists())
          f.remove(false);
    }
  },

  cancelDownload: function dv_cancelDownload(aItem) {
    this._dlmgr.cancelDownload(aItem.getAttribute("downloadID"));
    let f = this._getLocalFile(aItem.getAttribute("file"));
    if (f.exists())
      f.remove(false);
  },

  pauseDownload: function dv_pauseDownload(aItem) {
    this._dlmgr.pauseDownload(aItem.getAttribute("downloadID"));
  },

  resumeDownload: function dv_resumeDownload(aItem) {
    this._dlmgr.resumeDownload(aItem.getAttribute("downloadID"));
  },

  retryDownload: function dv_retryDownload(aItem) {
    this._removeItem(aItem);
    this._dlmgr.retryDownload(aItem.getAttribute("downloadID"));
  },

  showPage: function dv_showPage(aItem) {
    let uri = this._getReferrerOrSource(aItem);
    if (uri)
      BrowserUI.newTab(uri);
  },

  showAlert: function dv_showAlert(aName, aMessage, aTitle, aIcon) {
    if (this.visible)
      return;

    var notifier = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);

    
    let observer = {
      observe: function (aSubject, aTopic, aData) {
        if (aTopic == "alertclickcallback")
          BrowserUI.showPanel("downloads-container");
      }
    };

    if (!aTitle)
      aTitle = Strings.browser.GetStringFromName("alertDownloads");
    if (!aIcon)
      aIcon = URI_GENERIC_ICON_DOWNLOAD;

    notifier.showAlertNotification(aIcon, aTitle, aMessage, true, "", observer, aName);
  },

  observe: function (aSubject, aTopic, aData) {
    if (aTopic == "download-manager-remove-download") {
      
      if (!aSubject) {
        
        this.getDownloads();
        return;
      }

      
      let id = aSubject.QueryInterface(Ci.nsISupportsPRUint32);
      let element = this.getElementForDownload(id.data);
      this._removeItem(element);

      
      this._ifEmptyShowMessage();
    }
    else {
      let download = aSubject.QueryInterface(Ci.nsIDownload);
      let msgKey = "";

      if (aTopic == "dl-start") {
        msgKey = "alertDownloadsStart";
        if (!this._progressAlert) {
          if (!this._dlmgr)
            this._dlmgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
          this._progressAlert = new AlertDownloadProgressListener();
          this._dlmgr.addListener(this._progressAlert);
        }
      } else if (aTopic == "dl-done") {
        msgKey = "alertDownloadsDone";
      }

      if (msgKey)
        this.showAlert(download.target.spec.replace("file:", "download:"),
                       Strings.browser.formatStringFromName(msgKey, [download.displayName], 1));
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



function DownloadProgressListener() { }

DownloadProgressListener.prototype = {
  
  
  onDownloadStateChange: function dlPL_onDownloadStateChange(aState, aDownload) {
    let state = aDownload.state;
    switch (state) {
      case Ci.nsIDownloadManager.DOWNLOAD_QUEUED:
        DownloadsView.downloadStarted(aDownload);
        break;

      case Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_POLICY:
        DownloadsView.downloadStarted(aDownload);
        
        
      case Ci.nsIDownloadManager.DOWNLOAD_FAILED:
      case Ci.nsIDownloadManager.DOWNLOAD_CANCELED:
      case Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_PARENTAL:
      case Ci.nsIDownloadManager.DOWNLOAD_DIRTY:
      case Ci.nsIDownloadManager.DOWNLOAD_FINISHED:
        DownloadsView.downloadCompleted(aDownload);
        break;
    }

    let element = DownloadsView.getElementForDownload(aDownload.id);

    
    let referrer = aDownload.referrer;
    if (referrer && element.getAttribute("referrer") != referrer.spec)
      element.setAttribute("referrer", referrer.spec);

    
    element.setAttribute("state", state);
    element.setAttribute("currBytes", aDownload.amountTransferred);
    element.setAttribute("maxBytes", aDownload.size);
    element.setAttribute("endTime", Date.now());

    
    DownloadsView._updateTime(element);
    DownloadsView._updateStatus(element);
  },

  onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress, aDownload) {
    let element = DownloadsView.getElementForDownload(aDownload.id);
    if (!element)
      return;

    
    if (aDownload.percentComplete == -1) {
      element.setAttribute("progressmode", "undetermined");
    }
    else {
      element.setAttribute("progressmode", "normal");
      element.setAttribute("progress", aDownload.percentComplete);
    }

    
    let event = document.createEvent("Events");
    event.initEvent("ValueChange", true, true);
    let progmeter = document.getAnonymousElementByAttribute(element, "anonid", "progressmeter");
    if (progmeter)
      progmeter.dispatchEvent(event);

    
    element.setAttribute("currBytes", aDownload.amountTransferred);
    element.setAttribute("maxBytes", aDownload.size);

    
    DownloadsView._updateStatus(element);
  },

  onStateChange: function(aWebProgress, aRequest, aState, aStatus, aDownload) { },
  onSecurityChange: function(aWebProgress, aRequest, aState, aDownload) { },

  
  
  QueryInterface: function (aIID) {
    if (!aIID.equals(Ci.nsIDownloadProgressListener) &&
        !aIID.equals(Ci.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
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
      DownloadsView.showAlert(aDownload.target.spec.replace("file:", "download:"),
                              strings.GetStringFromName("alertDownloadsNoSpace"),
                              strings.GetStringFromName("alertDownloadsSize"));

      Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager).cancelDownload(aDownload.id);
    }

#ifdef ANDROID
    if (aDownload.percentComplete == -1) {
      
      return;
    }
    let alertsService = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    let progressListener = alertsService.QueryInterface(Ci.nsIAlertsProgressListener);
    let notificationName = aDownload.target.spec.replace("file:", "download:");
    progressListener.onProgress(notificationName, aDownload.percentComplete, 100);
#endif
  },

  onDownloadStateChange: function(aState, aDownload) {
#ifdef ANDROID
    let state = aDownload.state;
    switch (state) {
      case Ci.nsIDownloadManager.DOWNLOAD_FAILED:
      case Ci.nsIDownloadManager.DOWNLOAD_CANCELED:
      case Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_PARENTAL:
      case Ci.nsIDownloadManager.DOWNLOAD_DIRTY:
      case Ci.nsIDownloadManager.DOWNLOAD_FINISHED: {
        let alertsService = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
        let progressListener = alertsService.QueryInterface(Ci.nsIAlertsProgressListener);
        let notificationName = aDownload.target.spec.replace("file:", "download:");
        progressListener.onCancel(notificationName);
        break;
      }
    }
#endif
  },

  onStateChange: function(aWebProgress, aRequest, aState, aStatus, aDownload) { },
  onSecurityChange: function(aWebProgress, aRequest, aState, aDownload) { },

  
  
  QueryInterface: function (aIID) {
    if (!aIID.equals(Ci.nsIDownloadProgressListener) &&
        !aIID.equals(Ci.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
