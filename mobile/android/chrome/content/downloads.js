






































const URI_GENERIC_ICON_DOWNLOAD = "drawable://alertdownloads";

var Downloads = {
  _initialized: false,
  _dlmgr: null,
  _progressAlert: null,

  _getLocalFile: function dl__getLocalFile(aFileURI) {
    
    
    const fileUrl = Services.io.newURI(aFileURI, null, null).QueryInterface(Ci.nsIFileURL);
    return fileUrl.file.clone().QueryInterface(Ci.nsILocalFile);
  },

  init: function dl_init() {
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
  },

  openDownload: function dl_openDownload(aFileURI) {
    let f = this._getLocalFile(aFileURI);
    try {
      f.launch();
    } catch (ex) { }
  },

  cancelDownload: function dl_cancelDownload(aDownload) {
    this._dlmgr.cancelDownload(aDownload.id);
    
    let fileURI = aDownload.target.spec;
    let f = this._getLocalFile(fileURI);
    if (f.exists())
      f.remove(false);
  },

  showAlert: function dl_showAlert(aDownload, aMessage, aTitle, aIcon) { 
    let self = this;

    
    let cancelPrompt = false;

    
    let observer = {
      observe: function (aSubject, aTopic, aData) {
        if (aTopic == "alertclickcallback") {
          if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_FINISHED) {
            
            self.openDownload(aDownload.target.spec);
          } else if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING &&
                     !cancelPrompt) {
            cancelPrompt = true;
            
            let title = Strings.browser.GetStringFromName("downloadCancelPromptTitle");
            let message = Strings.browser.GetStringFromName("downloadCancelPromptMessage");
            let flags = Services.prompt.BUTTON_POS_0 * Services.prompt.BUTTON_TITLE_YES +
                        Services.prompt.BUTTON_POS_1 * Services.prompt.BUTTON_TITLE_NO;

            let choice = Services.prompt.confirmEx(null, title, message, flags,
                                                   null, null, null, null, {});
            if (choice == 0)
              self.cancelDownload(aDownload);
            cancelPrompt = false;
          }
        }
      }
    };

    if (!aTitle)
      aTitle = Strings.browser.GetStringFromName("alertDownloads");
    if (!aIcon)
      aIcon = URI_GENERIC_ICON_DOWNLOAD;

    var notifier = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    notifier.showAlertNotification(aIcon, aTitle, aMessage, true, "", observer,
                                   aDownload.target.spec.replace("file:", "download:"));
  },

  observe: function dl_observe(aSubject, aTopic, aData) {
    let msgKey = "";
    if (aTopic == "dl-start") {
      msgKey = "alertDownloadsStart";
      if (!this._progressAlert) {
        if (!this._dlmgr)
          this._dlmgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
        this._progressAlert = new AlertDownloadProgressListener();
        this._dlmgr.addListener(this._progressAlert);

        NativeWindow.toast.show(Strings.browser.GetStringFromName("alertDownloadsToast"), "long");
      }
    } else if (aTopic == "dl-done") {
      msgKey = "alertDownloadsDone";
    }

    if (msgKey) {
      let download = aSubject.QueryInterface(Ci.nsIDownload);
      this.showAlert(download, Strings.browser.formatStringFromName(msgKey, [download.displayName], 1));
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
      Downloads.showAlert(aDownload, strings.GetStringFromName("alertDownloadsNoSpace"),
                                     strings.GetStringFromName("alertDownloadsSize"));

      Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager).cancelDownload(aDownload.id);
    }

    if (aDownload.percentComplete == -1) {
      
      return;
    }
    let alertsService = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    let progressListener = alertsService.QueryInterface(Ci.nsIAlertsProgressListener);
    let notificationName = aDownload.target.spec.replace("file:", "download:");
    progressListener.onProgress(notificationName, aDownload.percentComplete, 100);
  },

  onDownloadStateChange: function(aState, aDownload) {
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
