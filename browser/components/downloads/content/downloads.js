































































"use strict";







const DownloadsPanel = {
  
  

  



  _state: 0,

  
  get kStateUninitialized() 0,
  
  get kStateHidden() 1,
  
  get kStateWaitingData() 2,
  

  get kStateWaitingAnchor() 3,
  
  get kStateShown() 4,

  


  get kDownloadsOverlay()
      "chrome:

  






  initialize(aCallback) {
    DownloadsCommon.log("Attempting to initialize DownloadsPanel for a window.");
    if (this._state != this.kStateUninitialized) {
      DownloadsCommon.log("DownloadsPanel is already initialized.");
      DownloadsOverlayLoader.ensureOverlayLoaded(this.kDownloadsOverlay,
                                                 aCallback);
      return;
    }
    this._state = this.kStateHidden;

    window.addEventListener("unload", this.onWindowUnload, false);

    
    
    DownloadsCommon.initializeAllDataLinks();

    
    
    DownloadsCommon.log("Ensuring DownloadsPanel overlay loaded.");
    DownloadsOverlayLoader.ensureOverlayLoaded(this.kDownloadsOverlay, () => {
      DownloadsViewController.initialize();
      DownloadsCommon.log("Attaching DownloadsView...");
      DownloadsCommon.getData(window).addView(DownloadsView);
      DownloadsCommon.getSummary(window, DownloadsView.kItemCountLimit)
                     .addView(DownloadsSummary);
      DownloadsCommon.log("DownloadsView attached - the panel for this window",
                          "should now see download items come in.");
      DownloadsPanel._attachEventListeners();
      DownloadsCommon.log("DownloadsPanel initialized.");
      aCallback();
    });
  },

  




  terminate() {
    DownloadsCommon.log("Attempting to terminate DownloadsPanel for a window.");
    if (this._state == this.kStateUninitialized) {
      DownloadsCommon.log("DownloadsPanel was never initialized. Nothing to do.");
      return;
    }

    window.removeEventListener("unload", this.onWindowUnload, false);

    
    this.hidePanel();

    DownloadsViewController.terminate();
    DownloadsCommon.getData(window).removeView(DownloadsView);
    DownloadsCommon.getSummary(window, DownloadsView.kItemCountLimit)
                   .removeView(DownloadsSummary);
    this._unattachEventListeners();

    this._state = this.kStateUninitialized;

    DownloadsSummary.active = false;
    DownloadsCommon.log("DownloadsPanel terminated.");
  },

  
  

  



  get panel() {
    
    
    let downloadsPanel = document.getElementById("downloadsPanel");
    if (!downloadsPanel)
      return null;

    delete this.panel;
    return this.panel = downloadsPanel;
  },

  





  showPanel() {
    DownloadsCommon.log("Opening the downloads panel.");

    if (this.isPanelShowing) {
      DownloadsCommon.log("Panel is already showing - focusing instead.");
      this._focusPanel();
      return;
    }

    this.initialize(() => {
      
      
      
      
      setTimeout(() => this._openPopupIfDataReady(), 0);
    });

    DownloadsCommon.log("Waiting for the downloads panel to appear.");
    this._state = this.kStateWaitingData;
  },

  



  hidePanel() {
    DownloadsCommon.log("Closing the downloads panel.");

    if (!this.isPanelShowing) {
      DownloadsCommon.log("Downloads panel is not showing - nothing to do.");
      return;
    }

    this.panel.hidePopup();

    
    
    
    this._state = this.kStateHidden;
    DownloadsCommon.log("Downloads panel is now closed.");
  },

  


  get isPanelShowing() {
    return this._state == this.kStateWaitingData ||
           this._state == this.kStateWaitingAnchor ||
           this._state == this.kStateShown;
  },

  


  get keyFocusing() {
    return this.panel.hasAttribute("keyfocus");
  },

  




  set keyFocusing(aValue) {
    if (aValue) {
      this.panel.setAttribute("keyfocus", "true");
      this.panel.addEventListener("mousemove", this);
    } else {
      this.panel.removeAttribute("keyfocus");
      this.panel.removeEventListener("mousemove", this);
    }
    return aValue;
  },

  



  handleEvent(aEvent) {
    if (aEvent.type == "mousemove") {
      this.keyFocusing = false;
    }
  },

  
  

  


  onViewLoadCompleted() {
    this._openPopupIfDataReady();
  },

  
  

  onWindowUnload() {
    
    DownloadsPanel.terminate();
  },

  onPopupShown(aEvent) {
    
    if (aEvent.target != aEvent.currentTarget) {
      return;
    }

    DownloadsCommon.log("Downloads panel has shown.");
    this._state = this.kStateShown;

    
    DownloadsCommon.getIndicatorData(window).attentionSuppressed = true;

    
    if (DownloadsView.richListBox.itemCount > 0 &&
        DownloadsView.richListBox.selectedIndex == -1) {
      DownloadsView.richListBox.selectedIndex = 0;
    }

    this._focusPanel();
  },

  onPopupHidden(aEvent) {
    
    if (aEvent.target != aEvent.currentTarget) {
      return;
    }

    DownloadsCommon.log("Downloads panel has hidden.");

    
    
    this.keyFocusing = false;

    
    DownloadsCommon.getIndicatorData(window).attentionSuppressed = false;

    
    DownloadsButton.releaseAnchor();

    
    this._state = this.kStateHidden;
  },

  
  

  


  showDownloadsHistory() {
    DownloadsCommon.log("Showing download history.");
    
    
    this.hidePanel();

    BrowserDownloadsUI();
  },

  
  

  




  _attachEventListeners() {
    
    this.panel.addEventListener("keydown", this._onKeyDown.bind(this), false);
    
    
    this.panel.addEventListener("keypress", this._onKeyPress.bind(this), false);
  },

  



  _unattachEventListeners() {
    this.panel.removeEventListener("keydown", this._onKeyDown.bind(this),
                                   false);
    this.panel.removeEventListener("keypress", this._onKeyPress.bind(this),
                                   false);
  },

  _onKeyPress(aEvent) {
    
    if (aEvent.altKey || aEvent.ctrlKey || aEvent.shiftKey || aEvent.metaKey) {
      return;
    }

    let richListBox = DownloadsView.richListBox;

    
    
    
    
    if ((aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_TAB ||
        aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_UP ||
        aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_DOWN) &&
        !this.keyFocusing) {
      this.keyFocusing = true;
      
      
      if (DownloadsView.richListBox.selectedIndex == -1) {
        DownloadsView.richListBox.selectedIndex = 0;
      }
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

  




  _onKeyDown(aEvent) {
    
    
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
      DownloadURL(uri.spec, name, document);
    } catch (ex) {}
  },

  



  _focusPanel() {
    
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

  


  _openPopupIfDataReady() {
    
    
    if (this._state != this.kStateWaitingData || DownloadsView.loading) {
      return;
    }

    this._state = this.kStateWaitingAnchor;

    
    
    DownloadsButton.getAnchor(anchor => {
      
      
      if (this._state != this.kStateWaitingAnchor) {
        return;
      }

      
      
      
      
      if (window.windowState == Ci.nsIDOMChromeWindow.STATE_MINIMIZED) {
        DownloadsButton.releaseAnchor();
        this._state = this.kStateHidden;
        return;
      }

      if (!anchor) {
        DownloadsCommon.error("Downloads button cannot be found.");
        return;
      }

      
      
      
      
      for (let viewItem of DownloadsView._visibleViewItems.values()) {
        viewItem.download.refresh().catch(Cu.reportError);
      }

      DownloadsCommon.log("Opening downloads panel popup.");
      this.panel.openPopup(anchor, "bottomcenter topright", 0, 0, false, null);
    });
  },
};








const DownloadsOverlayLoader = {
  



  _loadRequests: [],

  


  _overlayLoading: false,

  


  _loadedOverlays: {},

  










  ensureOverlayLoaded(aOverlay, aCallback) {
    
    if (aOverlay in this._loadedOverlays) {
      aCallback();
      return;
    }

    
    this._loadRequests.push({ overlay: aOverlay, callback: aCallback });
    if (this._overlayLoading) {
      return;
    }

    this._overlayLoading = true;
    DownloadsCommon.log("Loading overlay ", aOverlay);
    document.loadOverlay(aOverlay, () => {
      this._overlayLoading = false;
      this._loadedOverlays[aOverlay] = true;

      this.processPendingRequests();
    });
  },

  




  processPendingRequests() {
    
    
    let currentLength = this._loadRequests.length;
    for (let i = 0; i < currentLength; i++) {
      let request = this._loadRequests.shift();

      
      
      
      this.ensureOverlayLoaded(request.overlay, request.callback);
    }
  },
};









const DownloadsView = {
  
  

  


  kItemCountLimit: 3,

  


  loading: false,

  





  _downloads: [],

  




  _visibleViewItems: new Map(),

  


  _itemCountChanged() {
    DownloadsCommon.log("The downloads item count has changed - we are tracking",
                        this._downloads.length, "downloads in total.");
    let count = this._downloads.length;
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

  


  get richListBox() {
    delete this.richListBox;
    return this.richListBox = document.getElementById("downloadsListBox");
  },

  


  get downloadsHistory() {
    delete this.downloadsHistory;
    return this.downloadsHistory = document.getElementById("downloadsHistory");
  },

  
  

  


  onDataLoadStarting() {
    DownloadsCommon.log("onDataLoadStarting called for DownloadsView.");
    this.loading = true;
  },

  


  onDataLoadCompleted() {
    DownloadsCommon.log("onDataLoadCompleted called for DownloadsView.");

    this.loading = false;

    
    
    this._itemCountChanged();

    
    
    DownloadsPanel.onViewLoadCompleted();
  },

  












  onDownloadAdded(download, aNewest) {
    DownloadsCommon.log("A new download data item was added - aNewest =",
                        aNewest);

    if (aNewest) {
      this._downloads.unshift(download);
    } else {
      this._downloads.push(download);
    }

    let itemsNowOverflow = this._downloads.length > this.kItemCountLimit;
    if (aNewest || !itemsNowOverflow) {
      
      
      
      this._addViewItem(download, aNewest);
    }
    if (aNewest && itemsNowOverflow) {
      
      
      this._removeViewItem(this._downloads[this.kItemCountLimit]);
    }

    
    
    if (!this.loading) {
      this._itemCountChanged();
    }
  },

  onDownloadStateChanged(download) {
    let viewItem = this._visibleViewItems.get(download);
    if (viewItem) {
      viewItem.onStateChanged();
    }
  },

  onDownloadChanged(download) {
    let viewItem = this._visibleViewItems.get(download);
    if (viewItem) {
      viewItem.onChanged();
    }
  },

  






  onDownloadRemoved(download) {
    DownloadsCommon.log("A download data item was removed.");

    let itemIndex = this._downloads.indexOf(download);
    this._downloads.splice(itemIndex, 1);

    if (itemIndex < this.kItemCountLimit) {
      
      this._removeViewItem(download);
      if (this._downloads.length >= this.kItemCountLimit) {
        
        this._addViewItem(this._downloads[this.kItemCountLimit - 1], false);
      }
    }

    this._itemCountChanged();
  },

  



  _controllersForElements: new Map(),

  controllerForElement(element) {
    return this._controllersForElements.get(element);
  },

  



  _addViewItem(download, aNewest)
  {
    DownloadsCommon.log("Adding a new DownloadsViewItem to the downloads list.",
                        "aNewest =", aNewest);

    let element = document.createElement("richlistitem");
    let viewItem = new DownloadsViewItem(download, element);
    this._visibleViewItems.set(download, viewItem);
    let viewItemController = new DownloadsViewItemController(download);
    this._controllersForElements.set(element, viewItemController);
    if (aNewest) {
      this.richListBox.insertBefore(element, this.richListBox.firstChild);
    } else {
      this.richListBox.appendChild(element);
    }
  },

  


  _removeViewItem(download) {
    DownloadsCommon.log("Removing a DownloadsViewItem from the downloads list.");
    let element = this._visibleViewItems.get(download).element;
    let previousSelectedIndex = this.richListBox.selectedIndex;
    this.richListBox.removeChild(element);
    if (previousSelectedIndex != -1) {
      this.richListBox.selectedIndex = Math.min(previousSelectedIndex,
                                                this.richListBox.itemCount - 1);
    }
    this._visibleViewItems.delete(download);
    this._controllersForElements.delete(element);
  },

  
  

  









  onDownloadCommand(aEvent, aCommand) {
    let target = aEvent.target;
    while (target.nodeName != "richlistitem") {
      target = target.parentNode;
    }
    DownloadsView.controllerForElement(target).doCommand(aCommand);
  },

  onDownloadClick(aEvent) {
    
    if (aEvent.button == 0 &&
        !aEvent.originalTarget.hasAttribute("oncommand")) {
      goDoCommand("downloadsCmd_open");
    }
  },

  


  onDownloadKeyPress(aEvent) {
    
    
    if (aEvent.originalTarget.hasAttribute("command") ||
        aEvent.originalTarget.hasAttribute("oncommand")) {
      return;
    }

    if (aEvent.charCode == " ".charCodeAt(0)) {
      goDoCommand("downloadsCmd_pauseResume");
      return;
    }

    if (aEvent.keyCode == KeyEvent.DOM_VK_RETURN) {
      goDoCommand("downloadsCmd_doDefault");
    }
  },

  


  onDownloadMouseOver(aEvent) {
    if (aEvent.originalTarget.parentNode == this.richListBox) {
      this.richListBox.selectedItem = aEvent.originalTarget;
    }
  },

  onDownloadMouseOut(aEvent) {
    if (aEvent.originalTarget.parentNode == this.richListBox) {
      
      
      let element = aEvent.relatedTarget;
      while (element && element != aEvent.originalTarget) {
        element = element.parentNode;
      }
      if (!element) {
        this.richListBox.selectedIndex = -1;
      }
    }
  },

  onDownloadContextMenu(aEvent) {
    let element = this.richListBox.selectedItem;
    if (!element) {
      return;
    }

    DownloadsViewController.updateCommands();

    
    let contextMenu = document.getElementById("downloadsContextMenu");
    contextMenu.setAttribute("state", element.getAttribute("state"));
  },

  onDownloadDragStart(aEvent) {
    let element = this.richListBox.selectedItem;
    if (!element) {
      return;
    }

    
    let file = new FileUtils.File(DownloadsView.controllerForElement(element)
                                               .download.target.path);
    if (!file.exists()) {
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
  },
}













function DownloadsViewItem(download, aElement) {
  this.download = download;
  this.element = aElement;
  this.element._shell = this;

  this.element.setAttribute("type", "download");
  this.element.classList.add("download-state");

  this._updateState();
}

DownloadsViewItem.prototype = {
  __proto__: DownloadElementShell.prototype,

  


  _element: null,

  onStateChanged() {
    this.element.setAttribute("image", this.image);
    this.element.setAttribute("state",
                              DownloadsCommon.stateOfDownload(this.download));
  },

  onChanged() {
    this._updateProgress();
  },
};









const DownloadsViewController = {
  
  

  initialize() {
    window.controllers.insertControllerAt(0, this);
  },

  terminate() {
    window.controllers.removeController(this);
  },

  
  

  supportsCommand(aCommand) {
    
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

  isCommandEnabled(aCommand) {
    
    if (aCommand == "downloadsCmd_clearList") {
      return DownloadsCommon.getData(window).canRemoveFinished;
    }

    
    let element = DownloadsView.richListBox.selectedItem;
    return element && DownloadsView.controllerForElement(element)
                                   .isCommandEnabled(aCommand);
  },

  doCommand(aCommand) {
    
    if (aCommand in this.commands) {
      this.commands[aCommand].apply(this);
      return;
    }

    
    let element = DownloadsView.richListBox.selectedItem;
    if (element) {
      
      DownloadsView.controllerForElement(element).doCommand(aCommand);
    }
  },

  onEvent() {},

  
  

  updateCommands() {
    Object.keys(this.commands).forEach(goUpdateCommand);
    Object.keys(DownloadsViewItemController.prototype.commands)
          .forEach(goUpdateCommand);
  },

  
  

  



  commands: {
    downloadsCmd_clearList() {
      DownloadsCommon.getData(window).removeFinished();
    }
  }
};








function DownloadsViewItemController(download) {
  this.download = download;
}

DownloadsViewItemController.prototype = {
  isCommandEnabled(aCommand) {
    switch (aCommand) {
      case "downloadsCmd_open": {
        if (!this.download.succeeded) {
          return false;
        }

        let file = new FileUtils.File(this.download.target.path);
        return file.exists();
      }
      case "downloadsCmd_show": {
        let file = new FileUtils.File(this.download.target.path);
        if (file.exists()) {
          return true;
        }

        if (!this.download.target.partFilePath) {
          return false;
        }

        let partFile = new FileUtils.File(this.download.target.partFilePath);
        return partFile.exists();
      }
      case "downloadsCmd_pauseResume":
        return this.download.hasPartialData && !this.download.error;
      case "downloadsCmd_retry":
        return this.download.canceled || this.download.error;
      case "downloadsCmd_openReferrer":
        return !!this.download.source.referrer;
      case "cmd_delete":
      case "downloadsCmd_cancel":
      case "downloadsCmd_copyLocation":
      case "downloadsCmd_doDefault":
        return true;
    }
    return false;
  },

  doCommand(aCommand) {
    if (this.isCommandEnabled(aCommand)) {
      this.commands[aCommand].apply(this);
    }
  },

  
  

  




  commands: {
    cmd_delete() {
      Downloads.getList(Downloads.ALL)
               .then(list => list.remove(this.download))
               .then(() => this.download.finalize(true))
               .catch(Cu.reportError);
      PlacesUtils.bhistory.removePage(
                             NetUtil.newURI(this.download.source.url));
    },

    downloadsCmd_cancel() {
      this.download.cancel().catch(() => {});
      this.download.removePartialData().catch(Cu.reportError);
    },

    downloadsCmd_open() {
      this.download.launch().catch(Cu.reportError);

      
      
      
      
      
      DownloadsPanel.hidePanel();
    },

    downloadsCmd_show() {
      let file = new FileUtils.File(this.download.target.path);
      DownloadsCommon.showDownloadedFile(file);

      
      
      
      
      
      DownloadsPanel.hidePanel();
    },

    downloadsCmd_pauseResume() {
      if (this.download.stopped) {
        this.download.start();
      } else {
        this.download.cancel();
      }
    },

    downloadsCmd_retry() {
      this.download.start().catch(() => {});
    },

    downloadsCmd_openReferrer() {
      openURL(this.download.source.referrer);
    },

    downloadsCmd_copyLocation() {
      let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"]
                      .getService(Ci.nsIClipboardHelper);
      clipboard.copyString(this.download.source.url, document);
    },

    downloadsCmd_doDefault() {
      const nsIDM = Ci.nsIDownloadManager;

      
      let defaultCommand = function () {
        switch (DownloadsCommon.stateOfDownload(this.download)) {
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
      if (defaultCommand && this.isCommandEnabled(defaultCommand)) {
        this.doCommand(defaultCommand);
      }
    },
  },
};









const DownloadsSummary = {

  






  set active(aActive) {
    if (aActive == this._active || !this._summaryNode) {
      return this._active;
    }
    if (aActive) {
      DownloadsCommon.getSummary(window, DownloadsView.kItemCountLimit)
                     .refreshView(this);
    } else {
      DownloadsFooter.showingSummary = false;
    }

    return this._active = aActive;
  },

  


  get active() this._active,

  _active: false,

  





  set showingProgress(aShowingProgress) {
    if (aShowingProgress) {
      this._summaryNode.setAttribute("inprogress", "true");
    } else {
      this._summaryNode.removeAttribute("inprogress");
    }
    
    return DownloadsFooter.showingSummary = aShowingProgress;
  },

  






  set percentComplete(aValue) {
    if (this._progressNode) {
      this._progressNode.setAttribute("value", aValue);
    }
    return aValue;
  },

  






  set description(aValue) {
    if (this._descriptionNode) {
      this._descriptionNode.setAttribute("value", aValue);
      this._descriptionNode.setAttribute("tooltiptext", aValue);
    }
    return aValue;
  },

  







  set details(aValue) {
    if (this._detailsNode) {
      this._detailsNode.setAttribute("value", aValue);
      this._detailsNode.setAttribute("tooltiptext", aValue);
    }
    return aValue;
  },

  


  focus() {
    if (this._summaryNode) {
      this._summaryNode.focus();
    }
  },

  





  onKeyDown(aEvent) {
    if (aEvent.charCode == " ".charCodeAt(0) ||
        aEvent.keyCode == KeyEvent.DOM_VK_RETURN) {
      DownloadsPanel.showDownloadsHistory();
    }
  },

  





  onClick(aEvent) {
    DownloadsPanel.showDownloadsHistory();
  },

  


  get _summaryNode() {
    let node = document.getElementById("downloadsSummary");
    if (!node) {
      return null;
    }
    delete this._summaryNode;
    return this._summaryNode = node;
  },

  


  get _progressNode() {
    let node = document.getElementById("downloadsSummaryProgress");
    if (!node) {
      return null;
    }
    delete this._progressNode;
    return this._progressNode = node;
  },

  



  get _descriptionNode() {
    let node = document.getElementById("downloadsSummaryDescription");
    if (!node) {
      return null;
    }
    delete this._descriptionNode;
    return this._descriptionNode = node;
  },

  



  get _detailsNode() {
    let node = document.getElementById("downloadsSummaryDetails");
    if (!node) {
      return null;
    }
    delete this._detailsNode;
    return this._detailsNode = node;
  }
}








const DownloadsFooter = {

  




  focus() {
    if (this._showingSummary) {
      DownloadsSummary.focus();
    } else {
      DownloadsView.downloadsHistory.focus();
    }
  },

  _showingSummary: false,

  



  set showingSummary(aValue) {
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

  


  get _footerNode() {
    let node = document.getElementById("downloadsFooter");
    if (!node) {
      return null;
    }
    delete this._footerNode;
    return this._footerNode = node;
  }
};
