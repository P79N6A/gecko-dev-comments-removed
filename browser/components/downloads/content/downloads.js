




































"use strict";




XPCOMUtils.defineLazyModuleGetter(this, "DownloadUtils",
                                  "resource://gre/modules/DownloadUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");







const DownloadsPanel = {
  
  

  



  _state: 0,

  
  get kStateUninitialized() 0,
  
  get kStateHidden() 1,
  
  get kStateShowing() 2,
  
  get kStateShown() 3,

  


  get kDownloadsOverlay()
      "chrome:

  






  initialize: function DP_initialize(aCallback)
  {
    if (this._state != this.kStateUninitialized) {
      DownloadsOverlayLoader.ensureOverlayLoaded(this.kDownloadsOverlay,
                                                 aCallback);
      return;
    }
    this._state = this.kStateHidden;

    window.addEventListener("unload", this.onWindowUnload, false);

    
    
    
    Services.downloads;

    
    
    DownloadsOverlayLoader.ensureOverlayLoaded(this.kDownloadsOverlay,
                                               function DP_I_callback() {
      DownloadsViewController.initialize();
      DownloadsCommon.data.addView(DownloadsView);
      aCallback();
    });
  },

  




  terminate: function DP_terminate()
  {
    if (this._state == this.kStateUninitialized) {
      return;
    }

    window.removeEventListener("unload", this.onWindowUnload, false);

    
    this.hidePanel();

    DownloadsViewController.terminate();
    DownloadsCommon.data.removeView(DownloadsView);

    this._state = this.kStateUninitialized;
  },

  
  

  


  get panel()
  {
    delete this.panel;
    return this.panel = document.getElementById("downloadsPanel");
  },

  





  showPanel: function DP_showPanel()
  {
    if (this.isPanelShowing) {
      this._focusPanel();
      return;
    }

    this.initialize(function DP_SP_callback() {
      
      
      
      
      setTimeout(function () DownloadsPanel._openPopupIfDataReady(), 0);
    }.bind(this));

    this._state = this.kStateShowing;
  },

  



  hidePanel: function DP_hidePanel()
  {
    if (!this.isPanelShowing) {
      return;
    }

    this.panel.hidePopup();

    
    
    
    this._state = this.kStateHidden;
  },

  


  get isPanelShowing()
  {
    return this._state == this.kStateShowing ||
           this._state == this.kStateShown;
  },

  
  

  


  onViewLoadCompleted: function DP_onViewLoadCompleted()
  {
    this._openPopupIfDataReady();
  },

  
  

  onWindowUnload: function DP_onWindowUnload()
  {
    
    DownloadsPanel.terminate();
  },

  onPopupShown: function DP_onPopupShown(aEvent)
  {
    
    if (aEvent.target != aEvent.currentTarget) {
      return;
    }

    this._state = this.kStateShown;

    
    DownloadsCommon.indicatorData.attentionSuppressed = true;

    
    if (DownloadsView.richListBox.itemCount > 0 &&
        !DownloadsView.richListBox.selectedItem) {
      DownloadsView.richListBox.selectedIndex = 0;
    }

    this._focusPanel();
  },

  onPopupHidden: function DP_onPopupHidden(aEvent)
  {
    
    if (aEvent.target != aEvent.currentTarget) {
      return;
    }

    
    DownloadsCommon.indicatorData.attentionSuppressed = false;

    
    DownloadsButton.releaseAnchor();

    
    this._state = this.kStateHidden;
  },

  
  

  


  showDownloadsHistory: function DP_showDownloadsHistory()
  {
    
    
    this.hidePanel();

    
    PlacesCommandHook.showPlacesOrganizer("Downloads");
  },

  
  

  



  _focusPanel: function DP_focusPanel()
  {
    
    if (this._state != this.kStateShown) {
      return;
    }

    let element = document.commandDispatcher.focusedElement;
    while (element && element != this.panel) {
      element = element.parentNode;
    }
    if (!element) {
      DownloadsView.richListBox.focus();
    }
  },

  


  _openPopupIfDataReady: function DP_openPopupIfDataReady()
  {
    
    
    if (this._state != this.kStateShowing || DownloadsView.loading) {
      return;
    }

    
    
    DownloadsButton.getAnchor(function DP_OPIDR_callback(aAnchor) {
      
      
      
      
      if (window.windowState == Ci.nsIDOMChromeWindow.STATE_MINIMIZED) {
        DownloadsButton.releaseAnchor();
        this._state = this.kStateHidden;
        return;
      }

      if (aAnchor) {
        this.panel.openPopup(aAnchor, "bottomcenter topright", 0, 0, false,
                             null);
      } else {
        this.panel.openPopup(document.getElementById("TabsToolbar"),
                             "after_end", 0, 0, false, null);
      }
    }.bind(this));
  }
};








const DownloadsOverlayLoader = {
  



  _loadRequests: [],

  


  _overlayLoading: false,

  


  _loadedOverlays: {},

  










  ensureOverlayLoaded: function DOL_ensureOverlayLoaded(aOverlay, aCallback)
  {
    
    if (aOverlay in this._loadedOverlays) {
      aCallback();
      return;
    }

    
    this._loadRequests.push({ overlay: aOverlay, callback: aCallback });
    if (this._overlayLoading) {
      return;
    }

    function DOL_EOL_loadCallback() {
      this._overlayLoading = false;
      this._loadedOverlays[aOverlay] = true;

      
      
      
      retrieveToolbarIconsizesFromTheme();

      this.processPendingRequests();
    }

    this._overlayLoading = true;
    document.loadOverlay(aOverlay, DOL_EOL_loadCallback.bind(this));
  },

  




  processPendingRequests: function DOL_processPendingRequests()
  {
    
    
    let currentLength = this._loadRequests.length;
    for (let i = 0; i < currentLength; i++) {
      let request = this._loadRequests.shift();

      
      
      
      this.ensureOverlayLoaded(request.overlay, request.callback);
    }
  }
};









const DownloadsView = {
  
  

  


  loading: false,

  



  _viewItems: {},

  


  _itemCountChanged: function DV_itemCountChanged()
  {
    if (Object.keys(this._viewItems).length > 0) {
      DownloadsPanel.panel.setAttribute("hasdownloads", "true");
    } else {
      DownloadsPanel.panel.removeAttribute("hasdownloads");
    }
  },

  


  get richListBox()
  {
    delete this.richListBox;
    return this.richListBox = document.getElementById("downloadsListBox");
  },

  
  

  


  onDataLoadStarting: function DV_onDataLoadStarting()
  {
    this.loading = true;
  },

  


  onDataLoadCompleted: function DV_onDataLoadCompleted()
  {
    this.loading = false;

    
    
    DownloadsPanel.onViewLoadCompleted();
  },

  




  onDataInvalidated: function DV_onDataInvalidated()
  {
    DownloadsPanel.terminate();

    
    let emptyView = this.richListBox.cloneNode(false);
    this.richListBox.parentNode.replaceChild(emptyView, this.richListBox);
    this.richListBox = emptyView;
    this._viewItems = {};
  },

  












  onDataItemAdded: function DV_onDataItemAdded(aDataItem, aNewest)
  {
    
    let element = document.createElement("richlistitem");
    let viewItem = new DownloadsViewItem(aDataItem, element);
    this._viewItems[aDataItem.downloadId] = viewItem;
    if (aNewest) {
      this.richListBox.insertBefore(element, this.richListBox.firstChild);
    } else {
      this.richListBox.appendChild(element);
    }

    this._itemCountChanged();
  },

  






  onDataItemRemoved: function DV_onDataItemRemoved(aDataItem)
  {
    let element = this.getViewItem(aDataItem)._element;
    let previousSelectedIndex = this.richListBox.selectedIndex;
    this.richListBox.removeChild(element);
    this.richListBox.selectedIndex = Math.min(previousSelectedIndex,
                                              this.richListBox.itemCount - 1);
    delete this._viewItems[aDataItem.downloadId];

    this._itemCountChanged();
  },

  







  getViewItem: function DV_getViewItem(aDataItem)
  {
    return this._viewItems[aDataItem.downloadId];
  },

  
  

  









  onDownloadCommand: function DV_onDownloadCommand(aEvent, aCommand)
  {
    let target = aEvent.target;
    while (target.nodeName != "richlistitem") {
      target = target.parentNode;
    }
    new DownloadsViewItemController(target).doCommand(aCommand);
  },

  onDownloadClick: function DV_onDownloadClick(aEvent)
  {
    
    if (aEvent.button == 0) {
      goDoCommand("downloadsCmd_open");
    }
  },

  onDownloadKeyPress: function DV_onDownloadKeyPress(aEvent)
  {
    
    if (aEvent.altKey || aEvent.ctrlKey || aEvent.shiftKey || aEvent.metaKey) {
      return;
    }

    
    
    if (aEvent.originalTarget.hasAttribute("command") ||
        aEvent.originalTarget.hasAttribute("oncommand")) {
      return;
    }

    if (aEvent.charCode == " ".charCodeAt(0)) {
      goDoCommand("downloadsCmd_pauseResume");
      return;
    }

    switch (aEvent.keyCode) {
      case KeyEvent.DOM_VK_ENTER:
      case KeyEvent.DOM_VK_RETURN:
        goDoCommand("downloadsCmd_doDefault");
        break;
    }
  },

  onDownloadContextMenu: function DV_onDownloadContextMenu(aEvent)
  {
    let element = this.richListBox.selectedItem;
    if (!element) {
      return;
    }

    DownloadsViewController.updateCommands();

    
    let contextMenu = document.getElementById("downloadsContextMenu");
    contextMenu.setAttribute("state", element.getAttribute("state"));
  },

  onDownloadDragStart: function DV_onDownloadDragStart(aEvent)
  {
    let element = this.richListBox.selectedItem;
    if (!element) {
      return;
    }

    let controller = new DownloadsViewItemController(element);
    let localFile = controller.dataItem.localFile;
    if (!localFile.exists()) {
      return;
    }

    let dataTransfer = aEvent.dataTransfer;
    dataTransfer.mozSetDataAt("application/x-moz-file", localFile, 0);
    dataTransfer.effectAllowed = "copyMove";
    dataTransfer.addElement(element);

    aEvent.stopPropagation();
  }
}













function DownloadsViewItem(aDataItem, aElement)
{
  this._element = aElement;
  this.dataItem = aDataItem;

  this.wasDone = this.dataItem.done;
  this.wasInProgress = this.dataItem.inProgress;
  this.lastEstimatedSecondsLeft = Infinity;

  
  
  
  
  this.image = "moz-icon://" + this.dataItem.file + "?size=32";

  let attributes = {
    "type": "download",
    "class": "download-state",
    "id": "downloadsItem_" + this.dataItem.downloadId,
    "downloadId": this.dataItem.downloadId,
    "state": this.dataItem.state,
    "progress": this.dataItem.inProgress ? this.dataItem.percentComplete : 100,
    "target": this.dataItem.target,
    "image": this.image
  };

  for (let attributeName in attributes) {
    this._element.setAttribute(attributeName, attributes[attributeName]);
  }

  
  this._updateProgress();
  this._updateStatusLine();
}

DownloadsViewItem.prototype = {
  


  dataItem: null,

  


  _element: null,

  


  _progressElement: null,

  
  

  




  onStateChange: function DVI_onStateChange()
  {
    
    
    
    
    
    
    if (!this.wasDone && this.dataItem.openable) {
      this._element.setAttribute("image", this.image + "&state=normal");
    }

    
    if (this.wasInProgress && !this.dataItem.inProgress) {
      this.endTime = Date.now();
    }

    this.wasDone = this.dataItem.done;
    this.wasInProgress = this.dataItem.inProgress;

    
    this._element.setAttribute("state", this.dataItem.state);
    this._updateProgress();
    this._updateStatusLine();
  },

  


  onProgressChange: function DVI_onProgressChange() {
    this._updateProgress();
    this._updateStatusLine();
  },

  
  

  


  _updateProgress: function DVI_updateProgress() {
    if (this.dataItem.starting) {
      
      this._element.setAttribute("progressmode", "normal");
      this._element.setAttribute("progress", "0");
    } else if (this.dataItem.state == Ci.nsIDownloadManager.DOWNLOAD_SCANNING ||
               this.dataItem.percentComplete == -1) {
      
      
      this._element.setAttribute("progressmode", "undetermined");
    } else {
      
      this._element.setAttribute("progressmode", "normal");
      this._element.setAttribute("progress", this.dataItem.percentComplete);
    }

    
    if (!this._progressElement) {
      this._progressElement =
           document.getAnonymousElementByAttribute(this._element, "anonid",
                                                   "progressmeter");
    }

    
    if (this._progressElement) {
      let event = document.createEvent("Events");
      event.initEvent("ValueChange", true, true);
      this._progressElement.dispatchEvent(event);
    }
  },

  



  _updateStatusLine: function DVI_updateStatusLine() {
    const nsIDM = Ci.nsIDownloadManager;

    let status = "";
    let statusTip = "";

    if (this.dataItem.paused) {
      let transfer = DownloadUtils.getTransferTotal(this.dataItem.currBytes,
                                                    this.dataItem.maxBytes);

      
      
      status = DownloadsCommon.strings.statusSeparatorBeforeNumber(
                                            DownloadsCommon.strings.statePaused,
                                            transfer);
    } else if (this.dataItem.state == nsIDM.DOWNLOAD_DOWNLOADING) {
      let newEstimatedSecondsLeft;
      [status, newEstimatedSecondsLeft] =
        DownloadUtils.getDownloadStatus(this.dataItem.currBytes,
                                        this.dataItem.maxBytes,
                                        this.dataItem.speed,
                                        this.lastEstimatedSecondsLeft);
      this.lastEstimatedSecondsLeft = newEstimatedSecondsLeft;
    } else if (this.dataItem.starting) {
      status = DownloadsCommon.strings.stateStarting;
    } else if (this.dataItem.state == nsIDM.DOWNLOAD_SCANNING) {
      status = DownloadsCommon.strings.stateScanning;
    } else if (!this.dataItem.inProgress) {
      let stateLabel = function () {
        let s = DownloadsCommon.strings;
        switch (this.dataItem.state) {
          case nsIDM.DOWNLOAD_FAILED:           return s.stateFailed;
          case nsIDM.DOWNLOAD_CANCELED:         return s.stateCanceled;
          case nsIDM.DOWNLOAD_BLOCKED_PARENTAL: return s.stateBlockedParentalControls;
          case nsIDM.DOWNLOAD_BLOCKED_POLICY:   return s.stateBlockedPolicy;
          case nsIDM.DOWNLOAD_DIRTY:            return s.stateDirty;
          case nsIDM.DOWNLOAD_FINISHED:         return this._fileSizeText;
        }
        return null;
      }.apply(this);

      let [displayHost, fullHost] =
        DownloadUtils.getURIHost(this.dataItem.referrer || this.dataItem.uri);

      let end = new Date(this.dataItem.endTime);
      let [displayDate, fullDate] = DownloadUtils.getReadableDates(end);

      
      
      
      
      let firstPart = DownloadsCommon.strings.statusSeparator(stateLabel,
                                                              displayHost);
      status = DownloadsCommon.strings.statusSeparator(firstPart, displayDate);
      statusTip = DownloadsCommon.strings.statusSeparator(fullHost, fullDate);
    }

    this._element.setAttribute("status", status);
    this._element.setAttribute("statusTip", statusTip || status);
  },

  



  get _fileSizeText()
  {
    
    let fileSize = this.dataItem.maxBytes;
    if (fileSize < 0) {
      return DownloadsCommon.strings.sizeUnknown;
    }
    let [size, unit] = DownloadUtils.convertByteUnits(fileSize);
    return DownloadsCommon.strings.sizeWithUnits(size, unit);
  }
};









const DownloadsViewController = {
  
  

  initialize: function DVC_initialize()
  {
    window.controllers.insertControllerAt(0, this);
  },

  terminate: function DVC_terminate()
  {
    window.controllers.removeController(this);
  },

  
  

  supportsCommand: function DVC_supportsCommand(aCommand)
  {
    
    if (!(aCommand in this.commands) &&
        !(aCommand in DownloadsViewItemController.prototype.commands)) {
      return false;
    }
    
    let element = document.commandDispatcher.focusedElement;
    while (element && element != DownloadsView.richListBox) {
      element = element.parentNode;
    }
    
    
    return !!element;
  },

  isCommandEnabled: function DVC_isCommandEnabled(aCommand)
  {
    
    if (aCommand == "downloadsCmd_clearList") {
      return Services.downloads.canCleanUp;
    }

    
    let element = DownloadsView.richListBox.selectedItem;
    return element &&
           new DownloadsViewItemController(element).isCommandEnabled(aCommand);
  },

  doCommand: function DVC_doCommand(aCommand)
  {
    
    if (aCommand in this.commands) {
      this.commands[aCommand].apply(this);
      return;
    }

    
    let element = DownloadsView.richListBox.selectedItem;
    if (element) {
      
      new DownloadsViewItemController(element).doCommand(aCommand);
    }
  },

  onEvent: function () { },

  
  

  updateCommands: function DVC_updateCommands()
  {
    Object.keys(this.commands).forEach(goUpdateCommand);
    Object.keys(DownloadsViewItemController.prototype.commands)
          .forEach(goUpdateCommand);
  },

  
  

  



  commands: {
    downloadsCmd_clearList: function DVC_downloadsCmd_clearList()
    {
      Services.downloads.cleanUp();
    }
  }
};








function DownloadsViewItemController(aElement) {
  let downloadId = aElement.getAttribute("downloadId");
  this.dataItem = DownloadsCommon.data.dataItems[downloadId];
}

DownloadsViewItemController.prototype = {
  
  

  get kPrefBdmAlertOnExeOpen() "browser.download.manager.alertOnEXEOpen",
  get kPrefBdmScanWhenDone() "browser.download.manager.scanWhenDone",

  
  

  


  dataItem: null,

  isCommandEnabled: function DVIC_isCommandEnabled(aCommand)
  {
    switch (aCommand) {
      case "downloadsCmd_open": {
        return this.dataItem.openable && this.dataItem.localFile.exists();
      }
      case "downloadsCmd_show": {
        return this.dataItem.localFile.exists();
      }
      case "downloadsCmd_pauseResume":
        return this.dataItem.inProgress && this.dataItem.resumable;
      case "downloadsCmd_retry":
        return this.dataItem.canRetry;
      case "downloadsCmd_openReferrer":
        return !!this.dataItem.referrer;
      case "cmd_delete":
      case "downloadsCmd_cancel":
      case "downloadsCmd_copyLocation":
      case "downloadsCmd_doDefault":
        return true;
    }
    return false;
  },

  doCommand: function DVIC_doCommand(aCommand)
  {
    if (this.isCommandEnabled(aCommand)) {
      this.commands[aCommand].apply(this);
    }
  },

  
  

  




  commands: {
    cmd_delete: function DVIC_cmd_delete()
    {
      this.commands.downloadsCmd_cancel.apply(this);

      Services.downloads.removeDownload(this.dataItem.downloadId);
    },

    downloadsCmd_cancel: function DVIC_downloadsCmd_cancel()
    {
      if (this.dataItem.inProgress) {
        Services.downloads.cancelDownload(this.dataItem.downloadId);

        
        
        try {
          let localFile = this.dataItem.localFile;
          if (localFile.exists()) {
            localFile.remove(false);
          }
        } catch (ex) { }
      }
    },

    downloadsCmd_open: function DVIC_downloadsCmd_open()
    {
      
      let localFile = this.dataItem.localFile;
      if (localFile.isExecutable()) {
        let showAlert = true;
        try {
          showAlert = Services.prefs.getBoolPref(this.kPrefBdmAlertOnExeOpen);
        } catch (ex) { }

        
        
        if (DownloadsCommon.isWinVistaOrHigher) {
          try {
            if (Services.prefs.getBoolPref(this.kPrefBdmScanWhenDone)) {
              showAlert = false;
            }
          } catch (ex) { }
        }

        if (showAlert) {
          let name = this.dataItem.target;
          let message =
              DownloadsCommon.strings.fileExecutableSecurityWarning(name, name);
          let title =
              DownloadsCommon.strings.fileExecutableSecurityWarningTitle;
          let dontAsk =
              DownloadsCommon.strings.fileExecutableSecurityWarningDontAsk;

          let checkbox = { value: false };
          let open = Services.prompt.confirmCheck(window, title, message,
                                                  dontAsk, checkbox);
          if (!open) {
            return;
          }

          Services.prefs.setBoolPref(this.kPrefBdmAlertOnExeOpen,
                                     !checkbox.value);
        }
      }

      
      try {
        let launched = false;
        try {
          let mimeInfo = this.dataItem.download.MIMEInfo;
          if (mimeInfo.preferredAction == mimeInfo.useHelperApp) {
            mimeInfo.launchWithFile(localFile);
            launched = true;
          }
        } catch (ex) { }
        if (!launched) {
          localFile.launch();
        }
      } catch (ex) {
        
        
        this._openExternal(localFile);
      }
    },

    downloadsCmd_show: function DVIC_downloadsCmd_show()
    {
      let localFile = this.dataItem.localFile;

      try {
        
        localFile.reveal();
      } catch (ex) {
        
        
        let parent = localFile.parent.QueryInterface(Ci.nsILocalFile);
        if (parent) {
          try {
            
            parent.launch();
          } catch (ex) {
            
            
            this._openExternal(parent);
          }
        }
      }
    },

    downloadsCmd_pauseResume: function DVIC_downloadsCmd_pauseResume()
    {
      if (this.dataItem.paused) {
        Services.downloads.resumeDownload(this.dataItem.downloadId);
      } else {
        Services.downloads.pauseDownload(this.dataItem.downloadId);
      }
    },

    downloadsCmd_retry: function DVIC_downloadsCmd_retry()
    {
      Services.downloads.retryDownload(this.dataItem.downloadId);
    },

    downloadsCmd_openReferrer: function DVIC_downloadsCmd_openReferrer()
    {
      openURL(this.dataItem.referrer);
    },

    downloadsCmd_copyLocation: function DVIC_downloadsCmd_copyLocation()
    {
      let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"]
                      .getService(Ci.nsIClipboardHelper);
      clipboard.copyString(this.dataItem.uri);
    },

    downloadsCmd_doDefault: function DVIC_downloadsCmd_doDefault()
    {
      const nsIDM = Ci.nsIDownloadManager;

      
      let defaultCommand = function () {
        switch (this.dataItem.state) {
          case nsIDM.DOWNLOAD_NOTSTARTED:       return "downloadsCmd_cancel";
          case nsIDM.DOWNLOAD_DOWNLOADING:      return "downloadsCmd_show";
          case nsIDM.DOWNLOAD_FINISHED:         return "downloadsCmd_open";
          case nsIDM.DOWNLOAD_FAILED:           return "downloadsCmd_retry";
          case nsIDM.DOWNLOAD_CANCELED:         return "downloadsCmd_retry";
          case nsIDM.DOWNLOAD_PAUSED:           return "downloadsCmd_pauseResume";
          case nsIDM.DOWNLOAD_QUEUED:           return "downloadsCmd_cancel";
          case nsIDM.DOWNLOAD_BLOCKED_PARENTAL: return "downloadsCmd_openReferrer";
          case nsIDM.DOWNLOAD_SCANNING:         return "downloadsCmd_show";
          case nsIDM.DOWNLOAD_DIRTY:            return "downloadsCmd_openReferrer";
          case nsIDM.DOWNLOAD_BLOCKED_POLICY:   return "downloadsCmd_openReferrer";
        }
        return null;
      }.apply(this);
      
      this.doCommand(defaultCommand);
    }
  },

  


  _openExternal: function DVIC_openExternal(aFile)
  {
    let protocolSvc = Cc["@mozilla.org/uriloader/external-protocol-service;1"]
                      .getService(Ci.nsIExternalProtocolService);
    protocolSvc.loadUrl(makeFileURI(aFile));
  }
};
