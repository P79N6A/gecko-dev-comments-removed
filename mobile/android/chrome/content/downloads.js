




"use strict";

function dump(a) {
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(a);
}

const URI_GENERIC_ICON_DOWNLOAD = "drawable://alert_download";

XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");

var Downloads = {
  _initialized: false,
  _dlmgr: null,
  _progressAlert: null,
  _privateDownloads: [],

  _getLocalFile: function dl__getLocalFile(aFileURI) {
    
    
    const fileUrl = Services.io.newURI(aFileURI, null, null).QueryInterface(Ci.nsIFileURL);
    return fileUrl.file.clone().QueryInterface(Ci.nsILocalFile);
  },

  init: function dl_init() {
    if (this._initialized)
      return;
    this._initialized = true;

    
    this._dlmgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
    this._progressAlert = new AlertDownloadProgressListener();
    this._dlmgr.addPrivacyAwareListener(this._progressAlert);
    Services.obs.addObserver(this, "last-pb-context-exited", true);
  },

  openDownload: function dl_openDownload(aDownload) {
    let fileUri = aDownload.target.spec;
    let guid = aDownload.guid;
    let f = this._getLocalFile(fileUri);
    try {
      f.launch();
    } catch (ex) {
      
      
      BrowserApp.addTab("about:downloads?id=" + guid);
    }
  },

  cancelDownload: function dl_cancelDownload(aDownload) {
    aDownload.cancel();
    
    let fileURI = aDownload.target.spec;
    let f = this._getLocalFile(fileURI);

    OS.File.remove(f.path);
  },

  showAlert: function dl_showAlert(aDownload, aMessage, aTitle, aIcon) { 
    let self = this;

    
    let cancelPrompt = false;

    
    let observer = {
      observe: function (aSubject, aTopic, aData) {
        if (aTopic == "alertclickcallback") {
          if (aDownload.state == Ci.nsIDownloadManager.DOWNLOAD_FINISHED) {
            
            self.openDownload(aDownload);
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

    if (!aIcon)
      aIcon = URI_GENERIC_ICON_DOWNLOAD;

    if (aDownload.isPrivate) {
      this._privateDownloads.push(aDownload);
    }

    var notifier = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    notifier.showAlertNotification(aIcon, aTitle, aMessage, true, "", observer,
                                   aDownload.target.spec.replace("file:", "download:"));
  },

  
  observe: function dl_observe(aSubject, aTopic, aData) {
    let alertsService = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    let progressListener = alertsService.QueryInterface(Ci.nsIAlertsProgressListener);
    let download;
    while ((download = this._privateDownloads.pop())) {
      try {
        let notificationName = download.target.spec.replace("file:", "download:");
        progressListener.onCancel(notificationName);
      } catch (e) {
        dump("Error removing private download: " + e);
      }
    }
  },

  QueryInterface: function (aIID) {
    if (!aIID.equals(Ci.nsISupports) &&
        !aIID.equals(Ci.nsIObserver) &&
        !aIID.equals(Ci.nsISupportsWeakReference))
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

      aDownload.cancel();
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
      case Ci.nsIDownloadManager.DOWNLOAD_QUEUED:
        NativeWindow.toast.show(Strings.browser.GetStringFromName("alertDownloadsToast"), "long");
        Downloads.showAlert(aDownload, Strings.browser.GetStringFromName("alertDownloadsStart2"),
                            aDownload.displayName);
        break;
      case Ci.nsIDownloadManager.DOWNLOAD_FAILED:
      case Ci.nsIDownloadManager.DOWNLOAD_CANCELED:
      case Ci.nsIDownloadManager.DOWNLOAD_BLOCKED_PARENTAL:
      case Ci.nsIDownloadManager.DOWNLOAD_DIRTY:
      case Ci.nsIDownloadManager.DOWNLOAD_FINISHED: {
        let alertsService = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
        let progressListener = alertsService.QueryInterface(Ci.nsIAlertsProgressListener);
        let notificationName = aDownload.target.spec.replace("file:", "download:");
        progressListener.onCancel(notificationName);

        if (aDownload.isPrivate) {
          let index = this._privateDownloads.indexOf(aDownload);
          if (index != -1) {
            this._privateDownloads.splice(index, 1);
          }
        }

        if (state == Ci.nsIDownloadManager.DOWNLOAD_FINISHED) {
          Downloads.showAlert(aDownload, Strings.browser.GetStringFromName("alertDownloadsDone2"),
                              aDownload.displayName);
        }
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
