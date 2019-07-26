































































"use strict";




XPCOMUtils.defineLazyModuleGetter(this, "DownloadUtils",
                                  "resource://gre/modules/DownloadUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");







const DownloadsPanel = {
  
  

  



  _state: 0,

  
  get kStateUninitialized() 0,
  
  get kStateHidden() 1,
  
  get kStateWaitingData() 2,
  

  get kStateWaitingAnchor() 3,
  
  get kStateShown() 4,

  


  get kDownloadsOverlay()
      "chrome:

  






  initialize: function DP_initialize(aCallback)
  {
    DownloadsCommon.log("Attempting to initialize DownloadsPanel for a window.");
    if (this._state != this.kStateUninitialized) {
      DownloadsCommon.log("DownloadsPanel is already initialized.");
      DownloadsOverlayLoader.ensureOverlayLoaded(this.kDownloadsOverlay,
                                                 aCallback);
      return;
    }
    this._state = this.kStateHidden;

    window.addEventListener("unload", this.onWindowUnload, false);

    
    
    
    Services.downloads;

    
    
    DownloadsCommon.log("Ensuring DownloadsPanel overlay loaded.");
    DownloadsOverlayLoader.ensureOverlayLoaded(this.kDownloadsOverlay,
                                               function DP_I_callback() {
      DownloadsViewController.initialize();
      DownloadsCommon.log("Attaching DownloadsView...");
      DownloadsCommon.getData(window).addView(DownloadsView);
      DownloadsCommon.log("DownloadsView attached - the panel for this window",
                          "should now see download items come in.");
      DownloadsPanel._attachEventListeners();
      DownloadsCommon.log("DownloadsPanel initialized.");
      aCallback();
    });
  },

  




  terminate: function DP_terminate()
  {
    DownloadsCommon.log("Attempting to terminate DownloadsPanel for a window.");
    if (this._state == this.kStateUninitialized) {
      DownloadsCommon.log("DownloadsPanel was never initialized. Nothing to do.");
      return;
    }

    window.removeEventListener("unload", this.onWindowUnload, false);

    
    this.hidePanel();

    DownloadsViewController.terminate();
    DownloadsCommon.getData(window).removeView(DownloadsView);
    this._unattachEventListeners();

    this._state = this.kStateUninitialized;

    DownloadsSummary.active = false;
    DownloadsCommon.log("DownloadsPanel terminated.");
  },

  
  

  


  get panel()
  {
    delete this.panel;
    return this.panel = document.getElementById("downloadsPanel");
  },

  





  showPanel: function DP_showPanel()
  {
    DownloadsCommon.log("Opening the downloads panel.");

    if (this.isPanelShowing) {
      DownloadsCommon.log("Panel is already showing - focusing instead.");
      this._focusPanel();
      return;
    }

    this.initialize(function DP_SP_callback() {
      
      
      
      
      setTimeout(function () DownloadsPanel._openPopupIfDataReady(), 0);
    }.bind(this));

    DownloadsCommon.log("Waiting for the downloads panel to appear.");
    this._state = this.kStateWaitingData;
  },

  



  hidePanel: function DP_hidePanel()
  {
    DownloadsCommon.log("Closing the downloads panel.");

    if (!this.isPanelShowing) {
      DownloadsCommon.log("Downloads panel is not showing - nothing to do.");
      return;
    }

    this.panel.hidePopup();

    
    
    
    this._state = this.kStateHidden;
    DownloadsCommon.log("Downloads panel is now closed.");
  },

  


  get isPanelShowing()
  {
    return this._state == this.kStateWaitingData ||
           this._state == this.kStateWaitingAnchor ||
           this._state == this.kStateShown;
  },

  


  get keyFocusing()
  {
    return this.panel.hasAttribute("keyfocus");
  },

  




  set keyFocusing(aValue)
  {
    if (aValue) {
      this.panel.setAttribute("keyfocus", "true");
      this.panel.addEventListener("mousemove", this);
    } else {
      this.panel.removeAttribute("keyfocus");
      this.panel.removeEventListener("mousemove", this);
    }
    return aValue;
  },

  



  handleEvent: function DP_handleEvent(aEvent)
  {
    if (aEvent.type == "mousemove") {
      this.keyFocusing = false;
    }
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

    DownloadsCommon.log("Downloads panel has shown.");
    this._state = this.kStateShown;

    
    DownloadsCommon.getIndicatorData(window).attentionSuppressed = true;

    
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

    DownloadsCommon.log("Downloads panel has hidden.");

    
    
    this.keyFocusing = false;

    
    DownloadsCommon.getIndicatorData(window).attentionSuppressed = false;

    
    DownloadsButton.releaseAnchor();

    
    this._state = this.kStateHidden;
  },

  
  

  


  showDownloadsHistory: function DP_showDownloadsHistory()
  {
    DownloadsCommon.log("Showing download history.");
    
    
    this.hidePanel();

    BrowserDownloadsUI();
  },

  
  

  




  _attachEventListeners: function DP__attachEventListeners()
  {
    
    this.panel.addEventListener("keydown", this._onKeyDown.bind(this), false);
    
    
    this.panel.addEventListener("keypress", this._onKeyPress.bind(this), false);
  },

  



  _unattachEventListeners: function DP__unattachEventListeners()
  {
    this.panel.removeEventListener("keydown", this._onKeyDown.bind(this),
                                   false);
    this.panel.removeEventListener("keypress", this._onKeyPress.bind(this),
                                   false);
  },

  _onKeyPress: function DP__onKeyPress(aEvent)
  {
    
    if (aEvent.altKey || aEvent.ctrlKey || aEvent.shiftKey || aEvent.metaKey) {
      return;
    }

    let richListBox = DownloadsView.richListBox;

    
    
    
    
    if ((aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_TAB ||
        aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_UP ||
        aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_DOWN) &&
        !this.keyFocusing) {
      this.keyFocusing = true;
      aEvent.preventDefault();
      return;
    }

    if (aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_DOWN) {
      
      
      if (richListBox.selectedItem === richListBox.lastChild ||
          document.activeElement.parentNode.id === "downloadsFooter") {
        DownloadsFooter.focus();
        aEvent.preventDefault();
        return;
      }
    }

    
    if (document.activeElement === richListBox) {
      DownloadsView.onDownloadKeyPress(aEvent);
    }
  },

  




  _onKeyDown: function DP__onKeyDown(aEvent)
  {
    
    
    if (aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_UP &&
        document.activeElement.parentNode.id === "downloadsFooter" &&
        DownloadsView.richListBox.firstChild) {
      DownloadsView.richListBox.focus();
      DownloadsView.richListBox.selectedItem = DownloadsView.richListBox.lastChild;
      aEvent.preventDefault();
      return;
    }

    let pasting = aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_V &&
#ifdef XP_MACOSX
                  aEvent.metaKey;
#else
                  aEvent.ctrlKey;
#endif

    if (!pasting) {
      return;
    }

    DownloadsCommon.log("Received a paste event.");

    let trans = Cc["@mozilla.org/widget/transferable;1"]
                  .createInstance(Ci.nsITransferable);
    trans.init(null);
    let flavors = ["text/x-moz-url", "text/unicode"];
    flavors.forEach(trans.addDataFlavor);
    Services.clipboard.getData(trans, Services.clipboard.kGlobalClipboard);
    
    try {
      let data = {};
      trans.getAnyTransferData({}, data, {});
      let [url, name] = data.value
                            .QueryInterface(Ci.nsISupportsString)
                            .data
                            .split("\n");
      if (!url) {
        return;
      }

      let uri = NetUtil.newURI(url);
      DownloadsCommon.log("Pasted URL seems valid. Starting download.");
      saveURL(uri.spec, name || uri.spec, null, true, true,
              undefined, document);
    } catch (ex) {}
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
      if (DownloadsView.richListBox.itemCount > 0) {
        DownloadsView.richListBox.focus();
      } else {
        DownloadsFooter.focus();
      }
    }
  },

  


  _openPopupIfDataReady: function DP_openPopupIfDataReady()
  {
    
    
    if (this._state != this.kStateWaitingData || DownloadsView.loading) {
      return;
    }

    this._state = this.kStateWaitingAnchor;

    
    
    DownloadsButton.getAnchor(function DP_OPIDR_callback(aAnchor) {
      
      
      if (this._state != this.kStateWaitingAnchor)
        return;

      
      
      
      
      if (window.windowState == Ci.nsIDOMChromeWindow.STATE_MINIMIZED) {
        DownloadsButton.releaseAnchor();
        this._state = this.kStateHidden;
        return;
      }

      if (aAnchor) {
        DownloadsCommon.log("Opening downloads panel popup.");
        this.panel.openPopup(aAnchor, "bottomcenter topright", 0, 0, false,
                             null);
      } else {
        DownloadsCommon.error("We can't find the anchor! Failure case - opening",
                              "downloads panel on TabsToolbar. We should never",
                              "get here!");
        Components.utils.reportError(
          "Downloads button cannot be found");
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
    DownloadsCommon.log("Loading overlay ", aOverlay);
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
  
  

  


  kItemCountLimit: 3,

  


  loading: false,

  





  _dataItems: [],

  




  _viewItems: {},

  


  _itemCountChanged: function DV_itemCountChanged()
  {
    DownloadsCommon.log("The downloads item count has changed - we are tracking",
                        this._dataItems.length, "downloads in total.");
    let count = this._dataItems.length;
    let hiddenCount = count - this.kItemCountLimit;

    if (count > 0) {
      DownloadsCommon.log("Setting the panel's hasdownloads attribute to true.");
      DownloadsPanel.panel.setAttribute("hasdownloads", "true");
    } else {
      DownloadsCommon.log("Removing the panel's hasdownloads attribute.");
      DownloadsPanel.panel.removeAttribute("hasdownloads");
    }

    
    
    
    DownloadsSummary.active = hiddenCount > 0;
  },

  


  get richListBox()
  {
    delete this.richListBox;
    return this.richListBox = document.getElementById("downloadsListBox");
  },

  


  get downloadsHistory()
  {
    delete this.downloadsHistory;
    return this.downloadsHistory = document.getElementById("downloadsHistory");
  },

  
  

  


  onDataLoadStarting: function DV_onDataLoadStarting()
  {
    DownloadsCommon.log("onDataLoadStarting called for DownloadsView.");
    this.loading = true;
  },

  


  onDataLoadCompleted: function DV_onDataLoadCompleted()
  {
    DownloadsCommon.log("onDataLoadCompleted called for DownloadsView.");

    this.loading = false;

    
    
    this._itemCountChanged();

    
    
    DownloadsPanel.onViewLoadCompleted();
  },

  




  onDataInvalidated: function DV_onDataInvalidated()
  {
    DownloadsCommon.log("Downloads data has been invalidated. Cleaning up",
                        "DownloadsView.");

    DownloadsPanel.terminate();

    
    let emptyView = this.richListBox.cloneNode(false);
    this.richListBox.parentNode.replaceChild(emptyView, this.richListBox);
    this.richListBox = emptyView;
    this._viewItems = {};
    this._dataItems = [];
  },

  












  onDataItemAdded: function DV_onDataItemAdded(aDataItem, aNewest)
  {
    DownloadsCommon.log("A new download data item was added - aNewest =",
                        aNewest);

    if (aNewest) {
      this._dataItems.unshift(aDataItem);
    } else {
      this._dataItems.push(aDataItem);
    }

    let itemsNowOverflow = this._dataItems.length > this.kItemCountLimit;
    if (aNewest || !itemsNowOverflow) {
      
      
      
      this._addViewItem(aDataItem, aNewest);
    }
    if (aNewest && itemsNowOverflow) {
      
      
      this._removeViewItem(this._dataItems[this.kItemCountLimit]);
    }

    
    
    if (!this.loading) {
      this._itemCountChanged();
    }
  },

  






  onDataItemRemoved: function DV_onDataItemRemoved(aDataItem)
  {
    DownloadsCommon.log("A download data item was removed.");

    let itemIndex = this._dataItems.indexOf(aDataItem);
    this._dataItems.splice(itemIndex, 1);

    if (itemIndex < this.kItemCountLimit) {
      
      this._removeViewItem(aDataItem);
      if (this._dataItems.length >= this.kItemCountLimit) {
        
        this._addViewItem(this._dataItems[this.kItemCountLimit - 1], false);
      }
    }

    this._itemCountChanged();
  },

  







  getViewItem: function DV_getViewItem(aDataItem)
  {
    
    
    if (aDataItem.downloadGuid in this._viewItems) {
      return this._viewItems[aDataItem.downloadGuid];
    }
    return this._invisibleViewItem;
  },

  


  _invisibleViewItem: Object.freeze({
    onStateChange: function () { },
    onProgressChange: function () { }
  }),

  



  _addViewItem: function DV_addViewItem(aDataItem, aNewest)
  {
    DownloadsCommon.log("Adding a new DownloadsViewItem to the downloads list.",
                        "aNewest =", aNewest);

    let element = document.createElement("richlistitem");
    let viewItem = new DownloadsViewItem(aDataItem, element);
    this._viewItems[aDataItem.downloadGuid] = viewItem;
    if (aNewest) {
      this.richListBox.insertBefore(element, this.richListBox.firstChild);
    } else {
      this.richListBox.appendChild(element);
    }
  },

  


  _removeViewItem: function DV_removeViewItem(aDataItem)
  {
    DownloadsCommon.log("Removing a DownloadsViewItem from the downloads list.");
    let element = this.getViewItem(aDataItem)._element;
    let previousSelectedIndex = this.richListBox.selectedIndex;
    this.richListBox.removeChild(element);
    this.richListBox.selectedIndex = Math.min(previousSelectedIndex,
                                              this.richListBox.itemCount - 1);
    delete this._viewItems[aDataItem.downloadGuid];
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
    
    if (aEvent.button == 0 &&
        !aEvent.originalTarget.hasAttribute("oncommand")) {
      goDoCommand("downloadsCmd_open");
    }
  },

  


  onDownloadKeyPress: function DV_onDownloadKeyPress(aEvent)
  {
    
    
    if (aEvent.originalTarget.hasAttribute("command") ||
        aEvent.originalTarget.hasAttribute("oncommand")) {
      return;
    }

    if (aEvent.charCode == " ".charCodeAt(0)) {
      goDoCommand("downloadsCmd_pauseResume");
      return;
    }

    if (aEvent.keyCode == KeyEvent.DOM_VK_ENTER ||
        aEvent.keyCode == KeyEvent.DOM_VK_RETURN) {
      goDoCommand("downloadsCmd_doDefault");
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
    var url = Services.io.newFileURI(localFile).spec;
    dataTransfer.setData("text/uri-list", url);
    dataTransfer.setData("text/plain", url);
    dataTransfer.addElement(element);

    aEvent.stopPropagation();
  }
}













function DownloadsViewItem(aDataItem, aElement)
{
  this._element = aElement;
  this.dataItem = aDataItem;

  this.lastEstimatedSecondsLeft = Infinity;

  
  
  
  
  this.image = "moz-icon://" + this.dataItem.file + "?size=32";

  let attributes = {
    "type": "download",
    "class": "download-state",
    "id": "downloadsItem_" + this.dataItem.downloadGuid,
    "downloadGuid": this.dataItem.downloadGuid,
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

  
  

  




  onStateChange: function DVI_onStateChange(aOldState)
  {
    
    
    
    
    
    
    if (aOldState != Ci.nsIDownloadManager.DOWNLOAD_FINISHED &&
        aOldState != this.dataItem.state) {
      this._element.setAttribute("image", this.image + "&state=normal");
    }

    
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
      
      
      
      [status] =
        DownloadUtils.getDownloadStatusNoRate(this.dataItem.currBytes,
                                              this.dataItem.maxBytes,
                                              this.dataItem.speed,
                                              this.lastEstimatedSecondsLeft);

      
      let newEstimatedSecondsLeft;
      [statusTip, newEstimatedSecondsLeft] =
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
      if (PrivateBrowsingUtils.isWindowPrivate(window)) {
        return Services.downloads.canCleanUpPrivate;
      } else {
        return Services.downloads.canCleanUp;
      }
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
      if (PrivateBrowsingUtils.isWindowPrivate(window)) {
        Services.downloads.cleanUpPrivate();
      } else {
        Services.downloads.cleanUp();
      }
    }
  }
};








function DownloadsViewItemController(aElement) {
  let downloadGuid = aElement.getAttribute("downloadGuid");
  this.dataItem = DownloadsCommon.getData(window).dataItems[downloadGuid];
}

DownloadsViewItemController.prototype = {
  
  

  


  dataItem: null,

  isCommandEnabled: function DVIC_isCommandEnabled(aCommand)
  {
    switch (aCommand) {
      case "downloadsCmd_open": {
        return this.dataItem.openable && this.dataItem.localFile.exists();
      }
      case "downloadsCmd_show": {
        return this.dataItem.localFile.exists() ||
               this.dataItem.partFile.exists();
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
      this.dataItem.remove();
      PlacesUtils.bhistory.removePage(NetUtil.newURI(this.dataItem.uri));
    },

    downloadsCmd_cancel: function DVIC_downloadsCmd_cancel()
    {
      this.dataItem.cancel();
    },

    downloadsCmd_open: function DVIC_downloadsCmd_open()
    {
      this.dataItem.openLocalFile(window);
      
      
      
      
      
      DownloadsPanel.hidePanel();
    },

    downloadsCmd_show: function DVIC_downloadsCmd_show()
    {
      this.dataItem.showLocalFile();

      
      
      
      
      
      DownloadsPanel.hidePanel();
    },

    downloadsCmd_pauseResume: function DVIC_downloadsCmd_pauseResume()
    {
      this.dataItem.togglePauseResume();
    },

    downloadsCmd_retry: function DVIC_downloadsCmd_retry()
    {
      this.dataItem.retry();
    },

    downloadsCmd_openReferrer: function DVIC_downloadsCmd_openReferrer()
    {
      openURL(this.dataItem.referrer);
    },

    downloadsCmd_copyLocation: function DVIC_downloadsCmd_copyLocation()
    {
      let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"]
                      .getService(Ci.nsIClipboardHelper);
      clipboard.copyString(this.dataItem.uri, document);
    },

    downloadsCmd_doDefault: function DVIC_downloadsCmd_doDefault()
    {
      const nsIDM = Ci.nsIDownloadManager;

      
      let defaultCommand = function () {
        switch (this.dataItem.state) {
          case nsIDM.DOWNLOAD_NOTSTARTED:       return "downloadsCmd_cancel";
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
        return "";
      }.apply(this);
      if (defaultCommand && this.isCommandEnabled(defaultCommand))
        this.doCommand(defaultCommand);
    }
  }
};









const DownloadsSummary = {

  






  set active(aActive)
  {
    if (aActive == this._active || !this._summaryNode) {
      return this._active;
    }
    if (aActive) {
      DownloadsCommon.getSummary(window, DownloadsView.kItemCountLimit)
                     .addView(this);
    } else {
      DownloadsCommon.getSummary(window, DownloadsView.kItemCountLimit)
                     .removeView(this);
      DownloadsFooter.showingSummary = false;
    }

    return this._active = aActive;
  },

  


  get active() this._active,

  _active: false,

  





  set showingProgress(aShowingProgress)
  {
    if (aShowingProgress) {
      this._summaryNode.setAttribute("inprogress", "true");
    } else {
      this._summaryNode.removeAttribute("inprogress");
    }
    
    return DownloadsFooter.showingSummary = aShowingProgress;
  },

  






  set percentComplete(aValue)
  {
    if (this._progressNode) {
      this._progressNode.setAttribute("value", aValue);
    }
    return aValue;
  },

  






  set description(aValue)
  {
    if (this._descriptionNode) {
      this._descriptionNode.setAttribute("value", aValue);
      this._descriptionNode.setAttribute("tooltiptext", aValue);
    }
    return aValue;
  },

  







  set details(aValue)
  {
    if (this._detailsNode) {
      this._detailsNode.setAttribute("value", aValue);
      this._detailsNode.setAttribute("tooltiptext", aValue);
    }
    return aValue;
  },

  


  focus: function()
  {
    if (this._summaryNode) {
      this._summaryNode.focus();
    }
  },

  





  onKeyDown: function DS_onKeyDown(aEvent)
  {
    if (aEvent.charCode == " ".charCodeAt(0) ||
        aEvent.keyCode == KeyEvent.DOM_VK_ENTER ||
        aEvent.keyCode == KeyEvent.DOM_VK_RETURN) {
      DownloadsPanel.showDownloadsHistory();
    }
  },

  





  onClick: function DS_onClick(aEvent)
  {
    DownloadsPanel.showDownloadsHistory();
  },

  


  get _summaryNode()
  {
    let node = document.getElementById("downloadsSummary");
    if (!node) {
      return null;
    }
    delete this._summaryNode;
    return this._summaryNode = node;
  },

  


  get _progressNode()
  {
    let node = document.getElementById("downloadsSummaryProgress");
    if (!node) {
      return null;
    }
    delete this._progressNode;
    return this._progressNode = node;
  },

  



  get _descriptionNode()
  {
    let node = document.getElementById("downloadsSummaryDescription");
    if (!node) {
      return null;
    }
    delete this._descriptionNode;
    return this._descriptionNode = node;
  },

  



  get _detailsNode()
  {
    let node = document.getElementById("downloadsSummaryDetails");
    if (!node) {
      return null;
    }
    delete this._detailsNode;
    return this._detailsNode = node;
  }
}








const DownloadsFooter = {

  




  focus: function DF_focus()
  {
    if (this._showingSummary) {
      DownloadsSummary.focus();
    } else {
      DownloadsView.downloadsHistory.focus();
    }
  },

  _showingSummary: false,

  



  set showingSummary(aValue)
  {
    if (this._footerNode) {
      if (aValue) {
        this._footerNode.setAttribute("showingsummary", "true");
      } else {
        this._footerNode.removeAttribute("showingsummary");
      }
      this._showingSummary = aValue;
    }
    return aValue;
  },

  


  get _footerNode()
  {
    let node = document.getElementById("downloadsFooter");
    if (!node) {
      return null;
    }
    delete this._footerNode;
    return this._footerNode = node;
  }
};
