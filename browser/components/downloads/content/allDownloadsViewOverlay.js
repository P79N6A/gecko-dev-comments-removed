



let { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DownloadUtils",
                                  "resource://gre/modules/DownloadUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadsViewUI",
                                  "resource:///modules/DownloadsViewUI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
                                  "resource:///modules/RecentWindow.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

const nsIDM = Ci.nsIDownloadManager;

const DESTINATION_FILE_URI_ANNO  = "downloads/destinationFileURI";
const DOWNLOAD_META_DATA_ANNO    = "downloads/metaData";

const DOWNLOAD_VIEW_SUPPORTED_COMMANDS =
 ["cmd_delete", "cmd_copy", "cmd_paste", "cmd_selectAll",
  "downloadsCmd_pauseResume", "downloadsCmd_cancel", "downloadsCmd_unblock",
  "downloadsCmd_confirmBlock", "downloadsCmd_open", "downloadsCmd_show",
  "downloadsCmd_retry", "downloadsCmd_openReferrer", "downloadsCmd_clearDownloads"];








function HistoryDownload(aPlacesNode) {
  
  this.source = {
    url: aPlacesNode.uri,
  };
  this.target = {
    path: undefined,
    exists: false,
    size: undefined,
  };

  
  
  this.endTime = aPlacesNode.time / 1000;
}

HistoryDownload.prototype = {
  


  updateFromMetaData(metaData) {
    try {
      this.target.path = Cc["@mozilla.org/network/protocol;1?name=file"]
                           .getService(Ci.nsIFileProtocolHandler)
                           .getFileFromURLSpec(metaData.targetFileSpec).path;
    } catch (ex) {
      this.target.path = undefined;
    }

    if ("state" in metaData) {
      this.succeeded = metaData.state == nsIDM.DOWNLOAD_FINISHED;
      this.error = metaData.state == nsIDM.DOWNLOAD_FAILED
                   ? { message: "History download failed." }
                   : metaData.state == nsIDM.DOWNLOAD_BLOCKED_PARENTAL
                   ? { becauseBlockedByParentalControls: true }
                   : metaData.state == nsIDM.DOWNLOAD_DIRTY
                   ? { becauseBlockedByReputationCheck: true }
                   : null;
      this.canceled = metaData.state == nsIDM.DOWNLOAD_CANCELED ||
                      metaData.state == nsIDM.DOWNLOAD_PAUSED;
      this.endTime = metaData.endTime;

      
      
      this.target.exists = true;
      this.target.size = metaData.fileSize;
    } else {
      
      
      
      
      
      
      
      
      
      
      this.succeeded = !this.target.path;
      this.error = this.target.path ? { message: "Unstarted download." } : null;
      this.canceled = false;

      
      this.exists = false;
      this.target.size = undefined;
    }
  },

  


  stopped: true,

  


  hasProgress: false,

  






  hasPartialData: false,

  









  start() {
    let browserWin = RecentWindow.getMostRecentBrowserWindow();
    let initiatingDoc = browserWin ? browserWin.document : document;

    
    let leafName = this.target.path ? OS.Path.basename(this.target.path) : null;
    DownloadURL(this.source.url, leafName, initiatingDoc);

    return Promise.resolve();
  },

  



  refresh: Task.async(function* () {
    try {
      this.target.size = (yield OS.File.stat(this.target.path)).size;
      this.target.exists = true;
    } catch (ex) {
      
      this.target.exists = false;
    }
  }),
};





















function HistoryDownloadElementShell(aSessionDownload, aHistoryDownload) {
  this.element = document.createElement("richlistitem");
  this.element._shell = this;

  this.element.classList.add("download");
  this.element.classList.add("download-state");

  if (aSessionDownload) {
    this.sessionDownload = aSessionDownload;
  }
  if (aHistoryDownload) {
    this.historyDownload = aHistoryDownload;
  }
}

HistoryDownloadElementShell.prototype = {
  __proto__: DownloadsViewUI.DownloadElementShell.prototype,

  





  ensureActive() {
    if (!this._active) {
      this._active = true;
      this.element.setAttribute("active", true);
      this._updateUI();
    }
  },
  get active() !!this._active,

  



  get download() this._sessionDownload || this._historyDownload,

  _sessionDownload: null,
  get sessionDownload() this._sessionDownload,
  set sessionDownload(aValue) {
    if (this._sessionDownload != aValue) {
      if (!aValue && !this._historyDownload) {
        throw new Error("Should always have either a Download or a HistoryDownload");
      }

      this._sessionDownload = aValue;

      this.ensureActive();
      this._updateUI();
    }
    return aValue;
  },

  _historyDownload: null,
  get historyDownload() this._historyDownload,
  set historyDownload(aValue) {
    if (this._historyDownload != aValue) {
      if (!aValue && !this._sessionDownload) {
        throw new Error("Should always have either a Download or a HistoryDownload");
      }

      this._historyDownload = aValue;

      
      
      if (!this._sessionDownload) {
        this._updateUI();
      }
    }
    return aValue;
  },

  _updateUI() {
    
    if (!this.active) {
      return;
    }

    
    this._targetFileChecked = false;

    this._updateState();
  },

  get statusTextAndTip() {
    let status = this.rawStatusTextAndTip;

    
    
    if (!this.download.stopped) {
      status.text = status.tip;
    }
    status.tip = "";

    return status;
  },

  onStateChanged() {
    this.element.setAttribute("image", this.image);
    this.element.setAttribute("state",
                              DownloadsCommon.stateOfDownload(this.download));

    if (this.element.selected) {
      goUpdateDownloadCommands();
    } else {
      goUpdateCommand("downloadsCmd_clearDownloads");
    }
  },

  onChanged() {
    
    
    
    this.element.classList.toggle("temporary-block",
                                  !!this.download.hasBlockedData);
    this._updateProgress();
  },

  
  isCommandEnabled(aCommand) {
    
    if (!this.active && aCommand != "cmd_delete") {
      return false;
    }
    switch (aCommand) {
      case "downloadsCmd_open":
        
        return this.download.target.exists;
      case "downloadsCmd_show":
        
        if (this._sessionDownload && this.download.target.partFilePath) {
          let partFile = new FileUtils.File(this.download.target.partFilePath);
          if (partFile.exists()) {
            return true;
          }
        }

        
        return this.download.target.exists;
      case "downloadsCmd_pauseResume":
        return this.download.hasPartialData && !this.download.error;
      case "downloadsCmd_retry":
        return this.download.canceled || this.download.error;
      case "downloadsCmd_openReferrer":
        return !!this.download.source.referrer;
      case "cmd_delete":
        
        return this.download.stopped;
      case "downloadsCmd_cancel":
        return !!this._sessionDownload;
      case "downloadsCmd_confirmBlock":
      case "downloadsCmd_unblock":
        return this.download.hasBlockedData;
    }
    return false;
  },

  
  doCommand(aCommand) {
    switch (aCommand) {
      case "downloadsCmd_open": {
        let file = new FileUtils.File(this.download.target.path);
        DownloadsCommon.openDownloadedFile(file, null, window);
        break;
      }
      case "downloadsCmd_show": {
        let file = new FileUtils.File(this.download.target.path);
        DownloadsCommon.showDownloadedFile(file);
        break;
      }
      case "downloadsCmd_openReferrer": {
        openURL(this.download.source.referrer);
        break;
      }
      case "downloadsCmd_cancel": {
        this.download.cancel().catch(() => {});
        this.download.removePartialData().catch(Cu.reportError);
        break;
      }
      case "cmd_delete": {
        if (this._sessionDownload) {
          DownloadsCommon.removeAndFinalizeDownload(this.download);
        }
        if (this._historyDownload) {
          let uri = NetUtil.newURI(this.download.source.url);
          PlacesUtils.bhistory.removePage(uri);
        }
        break;
      }
      case "downloadsCmd_retry": {
        
        this.download.start().catch(() => {});
        break;
      }
      case "downloadsCmd_pauseResume": {
        
        if (this.download.stopped) {
          this.download.start();
        } else {
          this.download.cancel();
        }
        break;
      }
      case "downloadsCmd_unblock": {
        DownloadsCommon.confirmUnblockDownload(DownloadsCommon.BLOCK_VERDICT_MALWARE,
                                               window).then((confirmed) => {
          if (confirmed) {
            return this.download.unblock();
          }
        }).catch(Cu.reportError);
        break;
      }
      case "downloadsCmd_confirmBlock": {
        this.download.confirmBlock().catch(Cu.reportError);
        break;
      }
    }
  },

  
  
  
  matchesSearchTerm(aTerm) {
    if (!aTerm) {
      return true;
    }
    aTerm = aTerm.toLowerCase();
    return this.displayName.toLowerCase().includes(aTerm) ||
           this.download.source.url.toLowerCase().includes(aTerm);
  },

  
  
  doDefaultCommand() {
    function getDefaultCommandForState(aState) {
      switch (aState) {
        case nsIDM.DOWNLOAD_FINISHED:
          return "downloadsCmd_open";
        case nsIDM.DOWNLOAD_PAUSED:
          return "downloadsCmd_pauseResume";
        case nsIDM.DOWNLOAD_NOTSTARTED:
        case nsIDM.DOWNLOAD_QUEUED:
          return "downloadsCmd_cancel";
        case nsIDM.DOWNLOAD_FAILED:
        case nsIDM.DOWNLOAD_CANCELED:
          return "downloadsCmd_retry";
        case nsIDM.DOWNLOAD_SCANNING:
          return "downloadsCmd_show";
        case nsIDM.DOWNLOAD_BLOCKED_PARENTAL:
        case nsIDM.DOWNLOAD_DIRTY:
        case nsIDM.DOWNLOAD_BLOCKED_POLICY:
          return "downloadsCmd_openReferrer";
      }
      return "";
    }
    let state = DownloadsCommon.stateOfDownload(this.download);
    let command = getDefaultCommandForState(state);
    if (command && this.isCommandEnabled(command)) {
      this.doCommand(command);
    }
  },

  





  onSelect() {
    if (!this.active) {
      return;
    }

    
    
    if (!this.download.target.path) {
      return;
    }

    
    
    if (!this._targetFileChecked) {
      this._checkTargetFileOnSelect().catch(Cu.reportError);
    }
  },

  _checkTargetFileOnSelect: Task.async(function* () {
    try {
      yield this.download.refresh();
    } finally {
      
      this._targetFileChecked = true;
    }

    
    if (this.element.selected) {
      goUpdateDownloadCommands();
    }

    
    
    this._updateProgress();
  }),
};













function DownloadsPlacesView(aRichListBox, aActive = true) {
  this._richlistbox = aRichListBox;
  this._richlistbox._placesView = this;
  window.controllers.insertControllerAt(0, this);

  
  this._downloadElementsShellsForURI = new Map();

  
  this._viewItemsForDownloads = new WeakMap();

  
  
  this._lastSessionDownloadElement = null;

  this._searchTerm = "";

  this._active = aActive;

  
  
  this._initiallySelectedElement = null;
  this._downloadsData = DownloadsCommon.getData(window.opener || window);
  this._downloadsData.addView(this);

  
  
  DownloadsCommon.getIndicatorData(window).attention = false;

  
  window.addEventListener("unload", () => {
    window.controllers.removeController(this);
    this._downloadsData.removeView(this);
    this.result = null;
  }, true);
  
  window.addEventListener("resize", () => {
    this._ensureVisibleElementsAreActive();
  }, true);
}

DownloadsPlacesView.prototype = {
  get associatedElement() this._richlistbox,

  get active() this._active,
  set active(val) {
    this._active = val;
    if (this._active)
      this._ensureVisibleElementsAreActive();
    return this._active;
  },

  























  get _cachedPlacesMetaData() {
    if (!this.__cachedPlacesMetaData) {
      this.__cachedPlacesMetaData = new Map();

      
      for (let result of PlacesUtils.annotations.getAnnotationsWithName(
                                                 DOWNLOAD_META_DATA_ANNO)) {
        try {
          this.__cachedPlacesMetaData.set(result.uri.spec,
                                          JSON.parse(result.annotationValue));
        } catch (ex) {}
      }

      
      for (let result of PlacesUtils.annotations.getAnnotationsWithName(
                                                 DESTINATION_FILE_URI_ANNO)) {
        let metaData = this.__cachedPlacesMetaData.get(result.uri.spec);
        if (!metaData) {
          metaData = {};
          this.__cachedPlacesMetaData.set(result.uri.spec, metaData);
        }
        metaData.targetFileSpec = result.annotationValue;
      }
    }

    return this.__cachedPlacesMetaData;
  },
  __cachedPlacesMetaData: null,

  









  _getPlacesMetaDataFor(spec) {
    let metaData = {};

    try {
      let uri = NetUtil.newURI(spec);
      try {
        metaData = JSON.parse(PlacesUtils.annotations.getPageAnnotation(
                                          uri, DOWNLOAD_META_DATA_ANNO));
      } catch (ex) {}
      metaData.targetFileSpec = PlacesUtils.annotations.getPageAnnotation(
                                            uri, DESTINATION_FILE_URI_ANNO);
    } catch (ex) {}

    return metaData;
  },

  
























  _addDownloadData(sessionDownload, aPlacesNode, aNewest = false,
                   aDocumentFragment = null) {
    let downloadURI = aPlacesNode ? aPlacesNode.uri
                                  : sessionDownload.source.url;
    let shellsForURI = this._downloadElementsShellsForURI.get(downloadURI);
    if (!shellsForURI) {
      shellsForURI = new Set();
      this._downloadElementsShellsForURI.set(downloadURI, shellsForURI);
    }

    
    
    
    
    
    
    
    
    if (sessionDownload) {
      this._cachedPlacesMetaData.delete(sessionDownload.source.url);
    }

    let newOrUpdatedShell = null;

    
    
    let shouldCreateShell = shellsForURI.size == 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (!shouldCreateShell &&
        sessionDownload && !this._viewItemsForDownloads.has(sessionDownload)) {
      
      
      
      shouldCreateShell = true;
      for (let shell of shellsForURI) {
        if (!shell.sessionDownload) {
          shouldCreateShell = false;
          shell.sessionDownload = sessionDownload;
          newOrUpdatedShell = shell;
          this._viewItemsForDownloads.set(sessionDownload, shell);
          break;
        }
      }
    }

    if (shouldCreateShell) {
      
      
      
      let historyDownload = null;
      if (aPlacesNode) {
        let metaData = this._cachedPlacesMetaData.get(aPlacesNode.uri) ||
                       this._getPlacesMetaDataFor(aPlacesNode.uri);
        historyDownload = new HistoryDownload(aPlacesNode);
        historyDownload.updateFromMetaData(metaData);
      }
      let shell = new HistoryDownloadElementShell(sessionDownload,
                                                  historyDownload);
      shell.element._placesNode = aPlacesNode;
      newOrUpdatedShell = shell;
      shellsForURI.add(shell);
      if (sessionDownload) {
        this._viewItemsForDownloads.set(sessionDownload, shell);
      }
    } else if (aPlacesNode) {
      
      
      
      
      
      
      
      
      
      
      
      for (let shell of shellsForURI) {
        if (!shell.historyDownload) {
          
          shell.historyDownload = new HistoryDownload(aPlacesNode);
        }
        shell.element._placesNode = aPlacesNode;
      }
    }

    if (newOrUpdatedShell) {
      if (aNewest) {
        this._richlistbox.insertBefore(newOrUpdatedShell.element,
                                       this._richlistbox.firstChild);
        if (!this._lastSessionDownloadElement) {
          this._lastSessionDownloadElement = newOrUpdatedShell.element;
        }
        
        
        
        this._richlistbox.ensureElementIsVisible(newOrUpdatedShell.element);
      } else if (sessionDownload) {
        let before = this._lastSessionDownloadElement ?
          this._lastSessionDownloadElement.nextSibling : this._richlistbox.firstChild;
        this._richlistbox.insertBefore(newOrUpdatedShell.element, before);
        this._lastSessionDownloadElement = newOrUpdatedShell.element;
      } else {
        let appendTo = aDocumentFragment || this._richlistbox;
        appendTo.appendChild(newOrUpdatedShell.element);
      }

      if (this.searchTerm) {
        newOrUpdatedShell.element.hidden =
          !newOrUpdatedShell.element._shell.matchesSearchTerm(this.searchTerm);
      }
    }

    
    
    if (!aDocumentFragment) {
      this._ensureVisibleElementsAreActive();
      goUpdateCommand("downloadsCmd_clearDownloads");
    }
  },

  _removeElement(aElement) {
    
    
    if ((aElement.nextSibling || aElement.previousSibling) &&
        this._richlistbox.selectedItems &&
        this._richlistbox.selectedItems.length == 1 &&
        this._richlistbox.selectedItems[0] == aElement) {
      this._richlistbox.selectItem(aElement.nextSibling ||
                                   aElement.previousSibling);
    }

    if (this._lastSessionDownloadElement == aElement) {
      this._lastSessionDownloadElement = aElement.previousSibling;
    }

    this._richlistbox.removeItemFromSelection(aElement);
    this._richlistbox.removeChild(aElement);
    this._ensureVisibleElementsAreActive();
    goUpdateCommand("downloadsCmd_clearDownloads");
  },

  _removeHistoryDownloadFromView(aPlacesNode) {
    let downloadURI = aPlacesNode.uri;
    let shellsForURI = this._downloadElementsShellsForURI.get(downloadURI);
    if (shellsForURI) {
      for (let shell of shellsForURI) {
        if (shell.sessionDownload) {
          shell.historyDownload = null;
        } else {
          this._removeElement(shell.element);
          shellsForURI.delete(shell);
          if (shellsForURI.size == 0)
            this._downloadElementsShellsForURI.delete(downloadURI);
        }
      }
    }
  },

  _removeSessionDownloadFromView(download) {
    let shells = this._downloadElementsShellsForURI
                     .get(download.source.url);
    if (shells.size == 0) {
      throw new Error("Should have had at leaat one shell for this uri");
    }

    let shell = this._viewItemsForDownloads.get(download);
    if (!shells.has(shell)) {
      throw new Error("Missing download element shell in shells list for url");
    }

    
    
    
    
    if (shells.size > 1 || !shell.historyDownload) {
      this._removeElement(shell.element);
      shells.delete(shell);
      if (shells.size == 0) {
        this._downloadElementsShellsForURI.delete(download.source.url);
      }
    } else {
      
      
      
      
      
      let url = shell.historyDownload.source.url;
      let metaData = this._getPlacesMetaDataFor(url);
      shell.historyDownload.updateFromMetaData(metaData);
      shell.sessionDownload = null;
      
      if (this._lastSessionDownloadElement == shell.element) {
        this._lastSessionDownloadElement = shell.element.previousSibling;
      } else {
        let before = this._lastSessionDownloadElement ?
          this._lastSessionDownloadElement.nextSibling : this._richlistbox.firstChild;
        this._richlistbox.insertBefore(shell.element, before);
      }
    }
  },

  _ensureVisibleElementsAreActive() {
    if (!this.active || this._ensureVisibleTimer ||
        !this._richlistbox.firstChild) {
      return;
    }

    this._ensureVisibleTimer = setTimeout(() => {
      delete this._ensureVisibleTimer;
      if (!this._richlistbox.firstChild) {
        return;
      }

      let rlbRect = this._richlistbox.getBoundingClientRect();
      let winUtils = window.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      let nodes = winUtils.nodesFromRect(rlbRect.left, rlbRect.top,
                                         0, rlbRect.width, rlbRect.height, 0,
                                         true, false);
      
      
      
      let firstVisibleNode, lastVisibleNode;
      for (let node of nodes) {
        if (node.localName === "richlistitem" && node._shell) {
          node._shell.ensureActive();
          
          firstVisibleNode = node;
          
          if (!lastVisibleNode) {
            lastVisibleNode = node;
          }
        }
      }

      
      
      
      let nodeBelowVisibleArea = lastVisibleNode && lastVisibleNode.nextSibling;
      if (nodeBelowVisibleArea && nodeBelowVisibleArea._shell) {
        nodeBelowVisibleArea._shell.ensureActive();
      }

      let nodeAboveVisibleArea = firstVisibleNode &&
                                 firstVisibleNode.previousSibling;
      if (nodeAboveVisibleArea && nodeAboveVisibleArea._shell) {
        nodeAboveVisibleArea._shell.ensureActive();
      }
    }, 10);
  },

  _place: "",
  get place() this._place,
  set place(val) {
    
    if (this._place == val) {
      
      this.searchTerm = "";
      return val;
    }

    this._place = val;

    let history = PlacesUtils.history;
    let queries = { }, options = { };
    history.queryStringToQueries(val, queries, { }, options);
    if (!queries.value.length) {
      queries.value = [history.getNewQuery()];
    }

    let result = history.executeQueries(queries.value, queries.value.length,
                                        options.value);
    result.addObserver(this, false);
    return val;
  },

  _result: null,
  get result() this._result,
  set result(val) {
    if (this._result == val) {
      return val;
    }

    if (this._result) {
      this._result.removeObserver(this);
      this._resultNode.containerOpen = false;
    }

    if (val) {
      this._result = val;
      this._resultNode = val.root;
      this._resultNode.containerOpen = true;
      this._ensureInitialSelection();
    } else {
      delete this._resultNode;
      delete this._result;
    }

    return val;
  },

  get selectedNodes() {
    return [for (element of this._richlistbox.selectedItems)
            if (element._placesNode)
            element._placesNode];
  },

  get selectedNode() {
    let selectedNodes = this.selectedNodes;
    return selectedNodes.length == 1 ? selectedNodes[0] : null;
  },

  get hasSelection() this.selectedNodes.length > 0,

  containerStateChanged(aNode, aOldState, aNewState) {
    this.invalidateContainer(aNode)
  },

  invalidateContainer(aContainer) {
    if (aContainer != this._resultNode) {
      throw new Error("Unexpected container node");
    }
    if (!aContainer.containerOpen) {
      throw new Error("Root container for the downloads query cannot be closed");
    }

    let suppressOnSelect = this._richlistbox.suppressOnSelect;
    this._richlistbox.suppressOnSelect = true;
    try {
      
      
      
      for (let i = this._richlistbox.childNodes.length - 1; i >= 0; --i) {
        let element = this._richlistbox.childNodes[i];
        if (element._placesNode) {
          this._removeHistoryDownloadFromView(element._placesNode);
        }
      }
    } finally {
      this._richlistbox.suppressOnSelect = suppressOnSelect;
    }

    if (aContainer.childCount > 0) {
      let elementsToAppendFragment = document.createDocumentFragment();
      for (let i = 0; i < aContainer.childCount; i++) {
        try {
          this._addDownloadData(null, aContainer.getChild(i), false,
                                elementsToAppendFragment);
        } catch (ex) {
          Cu.reportError(ex);
        }
      }

      
      
      if (elementsToAppendFragment.firstChild) {
        this._appendDownloadsFragment(elementsToAppendFragment);
        this._ensureVisibleElementsAreActive();
      }
    }

    goUpdateDownloadCommands();
  },

  _appendDownloadsFragment(aDOMFragment) {
    
    

    
    
    let xblFields = new Map();
    for (let [key, value] in Iterator(this._richlistbox)) {
      xblFields.set(key, value);
    }

    let parentNode = this._richlistbox.parentNode;
    let nextSibling = this._richlistbox.nextSibling;
    parentNode.removeChild(this._richlistbox);
    this._richlistbox.appendChild(aDOMFragment);
    parentNode.insertBefore(this._richlistbox, nextSibling);

    for (let [key, value] of xblFields) {
      this._richlistbox[key] = value;
    }
  },

  nodeInserted(aParent, aPlacesNode) {
    this._addDownloadData(null, aPlacesNode);
  },

  nodeRemoved(aParent, aPlacesNode, aOldIndex) {
    this._removeHistoryDownloadFromView(aPlacesNode);
  },

  nodeAnnotationChanged() {},
  nodeIconChanged() {},
  nodeTitleChanged() {},
  nodeKeywordChanged() {},
  nodeDateAddedChanged() {},
  nodeLastModifiedChanged() {},
  nodeHistoryDetailsChanged() {},
  nodeTagsChanged() {},
  sortingChanged() {},
  nodeMoved() {},
  nodeURIChanged() {},
  batching() {},

  get controller() this._richlistbox.controller,

  get searchTerm() this._searchTerm,
  set searchTerm(aValue) {
    if (this._searchTerm != aValue) {
      for (let element of this._richlistbox.childNodes) {
        element.hidden = !element._shell.matchesSearchTerm(aValue);
      }
      this._ensureVisibleElementsAreActive();
    }
    return this._searchTerm = aValue;
  },

  














  _ensureInitialSelection() {
    
    if (this._richlistbox.selectedItem == this._initiallySelectedElement) {
      let firstDownloadElement = this._richlistbox.firstChild;
      if (firstDownloadElement != this._initiallySelectedElement) {
        
        
        
        
        firstDownloadElement._shell.ensureActive();
        Services.tm.mainThread.dispatch(() => {
          this._richlistbox.selectedItem = firstDownloadElement;
          this._richlistbox.currentItem = firstDownloadElement;
          this._initiallySelectedElement = firstDownloadElement;
        }, Ci.nsIThread.DISPATCH_NORMAL);
      }
    }
  },

  onDataLoadStarting() {},
  onDataLoadCompleted() {
    this._ensureInitialSelection();
  },

  onDownloadAdded(download, newest) {
    this._addDownloadData(download, null, newest);
  },

  onDownloadStateChanged(download) {
    this._viewItemsForDownloads.get(download).onStateChanged();
  },

  onDownloadChanged(download) {
    this._viewItemsForDownloads.get(download).onChanged();
  },

  onDownloadRemoved(download) {
    this._removeSessionDownloadFromView(download);
  },

  supportsCommand(aCommand) {
    if (DOWNLOAD_VIEW_SUPPORTED_COMMANDS.indexOf(aCommand) != -1) {
      
      
      
      
      
      
      
      
      if (document.activeElement == this._richlistbox ||
          aCommand == "downloadsCmd_clearDownloads") {
        return true;
      }
    }
    return false;
  },

  isCommandEnabled(aCommand) {
    switch (aCommand) {
      case "cmd_copy":
        return this._richlistbox.selectedItems.length > 0;
      case "cmd_selectAll":
        return true;
      case "cmd_paste":
        return this._canDownloadClipboardURL();
      case "downloadsCmd_clearDownloads":
        return this._canClearDownloads();
      default:
        return Array.every(this._richlistbox.selectedItems,
                           element => element._shell.isCommandEnabled(aCommand));
    }
  },

  _canClearDownloads() {
    
    
    
    
    for (let elt = this._richlistbox.lastChild; elt; elt = elt.previousSibling) {
      
      let download = elt._shell.download;
      if (download.stopped && !(download.canceled && download.hasPartialData)) {
        return true;
      }
    }
    return false;
  },

  _copySelectedDownloadsToClipboard() {
    let urls = [for (element of this._richlistbox.selectedItems)
                element._shell.download.source.url];

    Cc["@mozilla.org/widget/clipboardhelper;1"]
      .getService(Ci.nsIClipboardHelper)
      .copyString(urls.join("\n"), document);
  },

  _getURLFromClipboardData() {
    let trans = Cc["@mozilla.org/widget/transferable;1"].
                createInstance(Ci.nsITransferable);
    trans.init(null);

    let flavors = ["text/x-moz-url", "text/unicode"];
    flavors.forEach(trans.addDataFlavor);

    Services.clipboard.getData(trans, Services.clipboard.kGlobalClipboard);

    
    try {
      let data = {};
      trans.getAnyTransferData({}, data, {});
      let [url, name] = data.value.QueryInterface(Ci.nsISupportsString)
                            .data.split("\n");
      if (url) {
        return [NetUtil.newURI(url, null, null).spec, name];
      }
    } catch (ex) {}

    return ["", ""];
  },

  _canDownloadClipboardURL() {
    let [url, name] = this._getURLFromClipboardData();
    return url != "";
  },

  _downloadURLFromClipboard() {
    let [url, name] = this._getURLFromClipboardData();
    let browserWin = RecentWindow.getMostRecentBrowserWindow();
    let initiatingDoc = browserWin ? browserWin.document : document;
    DownloadURL(url, name, initiatingDoc);
  },

  doCommand(aCommand) {
    switch (aCommand) {
      case "cmd_copy":
        this._copySelectedDownloadsToClipboard();
        break;
      case "cmd_selectAll":
        this._richlistbox.selectAll();
        break;
      case "cmd_paste":
        this._downloadURLFromClipboard();
        break;
      case "downloadsCmd_clearDownloads":
        this._downloadsData.removeFinished();
        if (this.result) {
          Cc["@mozilla.org/browser/download-history;1"]
            .getService(Ci.nsIDownloadHistory)
            .removeAllDownloads();
        }
        
        
        goUpdateCommand("downloadsCmd_clearDownloads");
        break;
      default: {
        
        
        
        
        let selectedElements = this._richlistbox.selectedItems.slice();
        for (let element of selectedElements) {
          element._shell.doCommand(aCommand);
        }
      }
    }
  },

  onEvent() {},

  onContextMenu(aEvent) {
    let element = this._richlistbox.selectedItem;
    if (!element || !element._shell) {
      return false;
    }

    
    let contextMenu = document.getElementById("downloadsContextMenu");
    let download = element._shell.download;
    contextMenu.setAttribute("state",
                             DownloadsCommon.stateOfDownload(download));
    contextMenu.classList.toggle("temporary-block",
                                 !!download.hasBlockedData);

    if (!download.stopped) {
      
      
      goUpdateCommand("downloadsCmd_pauseResume");
    }

    return true;
  },

  onKeyPress(aEvent) {
    let selectedElements = this._richlistbox.selectedItems;
    if (aEvent.keyCode == KeyEvent.DOM_VK_RETURN) {
      
      
      
      if (selectedElements.length == 1) {
        let element = selectedElements[0];
        if (element._shell) {
          element._shell.doDefaultCommand();
        }
      }
    }
    else if (aEvent.charCode == " ".charCodeAt(0)) {
      
      for (let element of selectedElements) {
        if (element._shell.isCommandEnabled("downloadsCmd_pauseResume")) {
          element._shell.doCommand("downloadsCmd_pauseResume");
        }
      }
    }
  },

  onDoubleClick(aEvent) {
    if (aEvent.button != 0) {
      return;
    }

    let selectedElements = this._richlistbox.selectedItems;
    if (selectedElements.length != 1) {
      return;
    }

    let element = selectedElements[0];
    if (element._shell) {
      element._shell.doDefaultCommand();
    }
  },

  onScroll() {
    this._ensureVisibleElementsAreActive();
  },

  onSelect() {
    goUpdateDownloadCommands();

    let selectedElements = this._richlistbox.selectedItems;
    for (let elt of selectedElements) {
      if (elt._shell) {
        elt._shell.onSelect();
      }
    }
  },

  onDragStart(aEvent) {
    
    
    let selectedItem = this._richlistbox.selectedItem;
    if (!selectedItem) {
      return;
    }

    let targetPath = selectedItem._shell.download.target.path;
    if (!targetPath) {
      return;
    }

    
    let file = new FileUtils.File(targetPath);
    if (!file.exists()) {
      return;
    }

    let dt = aEvent.dataTransfer;
    dt.mozSetDataAt("application/x-moz-file", file, 0);
    let url = Services.io.newFileURI(file).spec;
    dt.setData("text/uri-list", url);
    dt.setData("text/plain", url);
    dt.effectAllowed = "copyMove";
    dt.addElement(selectedItem);
  },

  onDragOver(aEvent) {
    let types = aEvent.dataTransfer.types;
    if (types.contains("text/uri-list") ||
        types.contains("text/x-moz-url") ||
        types.contains("text/plain")) {
      aEvent.preventDefault();
    }
  },

  onDrop(aEvent) {
    let dt = aEvent.dataTransfer;
    
    
    if (dt.mozGetDataAt("application/x-moz-file", 0)) {
      return;
    }

    let name = {};
    let url = Services.droppedLinkHandler.dropLink(aEvent, name);
    if (url) {
      let browserWin = RecentWindow.getMostRecentBrowserWindow();
      let initiatingDoc = browserWin ? browserWin.document : document;
      DownloadURL(url, name.value, initiatingDoc);
    }
  },
};

for (let methodName of ["load", "applyFilter", "selectNode", "selectItems"]) {
  DownloadsPlacesView.prototype[methodName] = function () {
    throw new Error("|" + methodName +
                    "| is not implemented by the downloads view.");
  }
}

function goUpdateDownloadCommands() {
  for (let command of DOWNLOAD_VIEW_SUPPORTED_COMMANDS) {
    goUpdateCommand(command);
  }
}
