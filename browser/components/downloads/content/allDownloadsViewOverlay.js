









let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/DownloadUtils.jsm");
Cu.import("resource:///modules/DownloadsCommon.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
                                  "resource:///modules/RecentWindow.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");

const nsIDM = Ci.nsIDownloadManager;

const DESTINATION_FILE_URI_ANNO  = "downloads/destinationFileURI";
const DOWNLOAD_META_DATA_ANNO    = "downloads/metaData";

const DOWNLOAD_VIEW_SUPPORTED_COMMANDS =
 ["cmd_delete", "cmd_copy", "cmd_paste", "cmd_selectAll",
  "downloadsCmd_pauseResume", "downloadsCmd_cancel",
  "downloadsCmd_open", "downloadsCmd_show", "downloadsCmd_retry",
  "downloadsCmd_openReferrer", "downloadsCmd_clearDownloads"];








function HistoryDownload(url) {
  
  this.source = { url };
  this.target = { path: undefined, size: undefined };
}

HistoryDownload.prototype = {
  



  start() {
    
    
    
    let browserWin = RecentWindow.getMostRecentBrowserWindow();
    let initiatingDoc = browserWin ? browserWin.document : document;

    
    let leafName = this.target.path ? OS.Path.basename(this.target.path) : null;
    DownloadURL(this.source.url, leafName, initiatingDoc);

    return Promise.resolve();
  },
};








function DownloadsHistoryDataItem(aPlacesNode) {
  this.download = new HistoryDownload(aPlacesNode.uri);

  
  
  this.endTime = aPlacesNode.time / 1000;
}

DownloadsHistoryDataItem.prototype = {
  __proto__: DownloadsDataItem.prototype,

  


  updateFromMetaData(aPlacesMetaData) {
    try {
      let targetFile = Cc["@mozilla.org/network/protocol;1?name=file"]
                         .getService(Ci.nsIFileProtocolHandler)
                         .getFileFromURLSpec(aPlacesMetaData.targetFileURISpec);
      this.download.target.path = targetFile.path;
    } catch (ex) {
      this.download.target.path = undefined;
    }

    try {
      let metaData = JSON.parse(aPlacesMetaData.jsonDetails);
      this.state = metaData.state;
      this.endTime = metaData.endTime;
      this.download.target.size = metaData.fileSize;
    } catch (ex) {
      
      
      
      
      
      
      
      
      
      
      this.state = this.download.target.path ? nsIDM.DOWNLOAD_FAILED
                                             : nsIDM.DOWNLOAD_FINISHED;
      this.download.target.size = undefined;
    }

    
    
    this.maxBytes = this.download.target.size;

    
    this.percentComplete = 100;
  },
};





















function DownloadElementShell(aSessionDataItem, aHistoryDataItem) {
  this._element = document.createElement("richlistitem");
  this._element._shell = this;

  this._element.classList.add("download");
  this._element.classList.add("download-state");

  if (aSessionDataItem) {
    this.sessionDataItem = aSessionDataItem;
  }
  if (aHistoryDataItem) {
    this.historyDataItem = aHistoryDataItem;
  }
}

DownloadElementShell.prototype = {
  


  get element() this._element,

  





  ensureActive() {
    if (!this._active) {
      this._active = true;
      this._element.setAttribute("active", true);
      this._updateUI();
    }
  },
  get active() !!this._active,

  



  get download() this.dataItem.download,

  



  get dataItem() this._sessionDataItem || this._historyDataItem,

  _sessionDataItem: null,
  get sessionDataItem() this._sessionDataItem,
  set sessionDataItem(aValue) {
    if (this._sessionDataItem != aValue) {
      if (!aValue && !this._historyDataItem) {
        throw new Error("Should always have either a dataItem or a historyDataItem");
      }

      this._sessionDataItem = aValue;

      this.ensureActive();
      this._updateUI();
    }
    return aValue;
  },

  _historyDataItem: null,
  get historyDataItem() this._historyDataItem,
  set historyDataItem(aValue) {
    if (this._historyDataItem != aValue) {
      if (!aValue && !this._sessionDataItem) {
        throw new Error("Should always have either a dataItem or a historyDataItem");
      }

      this._historyDataItem = aValue;

      
      
      if (!this._sessionDataItem) {
        this._updateUI();
      }
    }
    return aValue;
  },

  
  get _progressElement() {
    if (!("__progressElement" in this)) {
      this.__progressElement =
        document.getAnonymousElementByAttribute(this._element, "anonid",
                                                "progressmeter");
    }
    return this.__progressElement;
  },

  _updateUI() {
    
    if (!this.active) {
      return;
    }

    
    this._targetFileChecked = false;

    this._element.setAttribute("displayName", this.displayName);
    this._element.setAttribute("image", this.image);

    this._updateActiveStatusUI();
  },

  
  
  
  _updateActiveStatusUI() {
    if (!this.active) {
      throw new Error("_updateActiveStatusUI called for an inactive item.");
    }

    this._element.setAttribute("state", this.dataItem.state);
    this._element.setAttribute("status", this.statusText);

    
    if (!this._sessionDataItem) {
      return;
    }

    
    if (this.dataItem.starting) {
      
      this._element.setAttribute("progressmode", "normal");
      this._element.setAttribute("progress", "0");
    } else if (this.dataItem.state == nsIDM.DOWNLOAD_SCANNING ||
               this.dataItem.percentComplete == -1) {
      
      
      this._element.setAttribute("progressmode", "undetermined");
    } else {
      
      this._element.setAttribute("progressmode", "normal");
      this._element.setAttribute("progress", this.dataItem.percentComplete);
    }

    
    if (this._progressElement) {
      let event = document.createEvent("Events");
      event.initEvent("ValueChange", true, true);
      this._progressElement.dispatchEvent(event);
    }
  },

  


  get image() {
    if (this.download.target.path) {
      return "moz-icon://" + this.download.target.path + "?size=32";
    }

    
    return "moz-icon://.unknown?size=32";
  },

  




  get displayName() {
    if (!this.download.target.path) {
      return this.download.source.url;
    }
    return OS.Path.basename(this.download.target.path);
  },

  get statusText() {
    let s = DownloadsCommon.strings;
    if (this.dataItem.inProgress) {
      if (this.dataItem.paused) {
        let transfer =
          DownloadUtils.getTransferTotal(this.download.currentBytes,
                                         this.dataItem.maxBytes);

         
         
         return s.statusSeparatorBeforeNumber(s.statePaused, transfer);
      }
      if (this.dataItem.state == nsIDM.DOWNLOAD_DOWNLOADING) {
        let [status, newEstimatedSecondsLeft] =
          DownloadUtils.getDownloadStatus(this.download.currentBytes,
                                          this.dataItem.maxBytes,
                                          this.download.speed,
                                          this._lastEstimatedSecondsLeft || Infinity);
        this._lastEstimatedSecondsLeft = newEstimatedSecondsLeft;
        return status;
      }
      if (this.dataItem.starting) {
        return s.stateStarting;
      }
      if (this.dataItem.state == nsIDM.DOWNLOAD_SCANNING) {
        return s.stateScanning;
      }

      throw new Error("_getStatusText called with a bogus download state");
    }

    
    let stateLabel = "";
    switch (this.dataItem.state) {
      case nsIDM.DOWNLOAD_FAILED:
        stateLabel = s.stateFailed;
        break;
      case nsIDM.DOWNLOAD_CANCELED:
        stateLabel = s.stateCanceled;
        break;
      case nsIDM.DOWNLOAD_BLOCKED_PARENTAL:
        stateLabel = s.stateBlockedParentalControls;
        break;
      case nsIDM.DOWNLOAD_BLOCKED_POLICY:
        stateLabel = s.stateBlockedPolicy;
        break;
      case nsIDM.DOWNLOAD_DIRTY:
        stateLabel = s.stateDirty;
        break;
      case nsIDM.DOWNLOAD_FINISHED:
        
        if (this.dataItem.maxBytes !== undefined) {
          let [size, unit] =
              DownloadUtils.convertByteUnits(this.dataItem.maxBytes);
          stateLabel = s.sizeWithUnits(size, unit);
          break;
        }
        
      default:
        stateLabel = s.sizeUnknown;
        break;
    }

    let referrer = this.download.source.referrer ||
                   this.download.source.url;
    let [displayHost, fullHost] = DownloadUtils.getURIHost(referrer);

    let date = new Date(this.dataItem.endTime);
    let [displayDate, fullDate] = DownloadUtils.getReadableDates(date);

    
    
    let firstPart = s.statusSeparator(stateLabel, displayHost);
    return s.statusSeparator(firstPart, displayDate);
  },

  onStateChanged() {
    
    
    
    
    
    
    if (this.dataItem.state == nsIDM.DOWNLOAD_FINISHED) {
      this._element.setAttribute("image", this.image + "&state=normal");
    }

    if (this._element.selected) {
      goUpdateDownloadCommands();
    } else {
      goUpdateCommand("downloadsCmd_clearDownloads");
    }
  },

  onChanged() {
    this._updateActiveStatusUI();
  },

  
  isCommandEnabled(aCommand) {
    
    if (!this.active && aCommand != "cmd_delete") {
      return false;
    }
    switch (aCommand) {
      case "downloadsCmd_open":
        
        
        
        if (this._sessionDataItem && !this.download.succeeded) {
          return false;
        }

        if (this._targetFileChecked) {
          return this._targetFileExists;
        }

        
        
        return this.dataItem.state == nsIDM.DOWNLOAD_FINISHED;
      case "downloadsCmd_show":
        
        if (this._sessionDataItem &&
            this.dataItem.partFile && this.dataItem.partFile.exists()) {
          return true;
        }

        if (this._targetFileChecked) {
          return this._targetFileExists;
        }

        
        
        return this.dataItem.state == nsIDM.DOWNLOAD_FINISHED;
      case "downloadsCmd_pauseResume":
        return this._sessionDataItem && this.dataItem.inProgress &&
               this.dataItem.download.hasPartialData;
      case "downloadsCmd_retry":
        return this.dataItem.canRetry;
      case "downloadsCmd_openReferrer":
        return !!this.download.source.referrer;
      case "cmd_delete":
        
        return !this.dataItem.inProgress;
      case "downloadsCmd_cancel":
        return !!this._sessionDataItem;
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
        if (this._sessionDataItem) {
          Downloads.getList(Downloads.ALL)
                   .then(list => list.remove(this.download))
                   .then(() => this.download.finalize(true))
                   .catch(Cu.reportError);
        }
        if (this._historyDataItem) {
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
    }
  },

  
  
  
  matchesSearchTerm(aTerm) {
    if (!aTerm) {
      return true;
    }
    aTerm = aTerm.toLowerCase();
    return this.displayName.toLowerCase().contains(aTerm) ||
           this.download.source.url.toLowerCase().contains(aTerm);
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
    let command = getDefaultCommandForState(this.dataItem.state);
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
      this._targetFileExists = yield OS.File.exists(this.download.target.path);
    } finally {
      
      this._targetFileChecked = true;
    }

    
    if (this._element.selected) {
      goUpdateDownloadCommands();
    }
  }),
};













function DownloadsPlacesView(aRichListBox, aActive = true) {
  this._richlistbox = aRichListBox;
  this._richlistbox._placesView = this;
  window.controllers.insertControllerAt(0, this);

  
  this._downloadElementsShellsForURI = new Map();

  
  this._viewItemsForDataItems = new WeakMap();

  
  
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

  



  _cachedPlacesMetaData: null,

  






















  _getCachedPlacesMetaDataFor(url) {
    if (!this._cachedPlacesMetaData) {
      this._cachedPlacesMetaData = new Map();

      
      for (let result of PlacesUtils.annotations.getAnnotationsWithName(
                                                 DESTINATION_FILE_URI_ANNO)) {
        this._cachedPlacesMetaData.set(result.uri.spec, {
          targetFileURISpec: result.annotationValue,
        });
      }

      
      for (let result of PlacesUtils.annotations.getAnnotationsWithName(
                                                 DOWNLOAD_META_DATA_ANNO)) {
        let metadata = this._cachedPlacesMetaData.get(result.uri.spec);

        
        
        
        if (metadata) {
          metadata.jsonDetails = result.annotationValue;
        }
      }
    }

    let metadata = this._cachedPlacesMetaData.get(url);
    this._cachedPlacesMetaData.delete(url);

    
    
    
    
    
    return metadata || this._getPlacesMetaDataFor(url);
  },

  








  _getPlacesMetaDataFor(url) {
    let metadata = {};

    try {
      let uri = NetUtil.newURI(url);
      metadata = {
        targetFileURISpec: PlacesUtils.annotations.getPageAnnotation(
                                       uri, DESTINATION_FILE_URI_ANNO),
      };
      metadata.jsonDetails = PlacesUtils.annotations.getPageAnnotation(
                                         uri, DOWNLOAD_META_DATA_ANNO);
    } catch (ex) {}

    return metadata;
  },

  


























  _addDownloadData(aDataItem, aPlacesNode, aNewest = false,
                   aDocumentFragment = null) {
    let downloadURI = aPlacesNode ? aPlacesNode.uri
                                  : aDataItem.download.source.url;
    let shellsForURI = this._downloadElementsShellsForURI.get(downloadURI);
    if (!shellsForURI) {
      shellsForURI = new Set();
      this._downloadElementsShellsForURI.set(downloadURI, shellsForURI);
    }

    let newOrUpdatedShell = null;

    
    
    let shouldCreateShell = shellsForURI.size == 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (!shouldCreateShell &&
        aDataItem && !this._viewItemsForDataItems.has(aDataItem)) {
      
      
      
      shouldCreateShell = true;
      for (let shell of shellsForURI) {
        if (!shell.sessionDataItem) {
          shouldCreateShell = false;
          shell.sessionDataItem = aDataItem;
          newOrUpdatedShell = shell;
          this._viewItemsForDataItems.set(aDataItem, shell);
          break;
        }
      }
    }

    if (shouldCreateShell) {
      
      
      
      let historyDataItem = null;
      if (aPlacesNode) {
        let metaData = this._getCachedPlacesMetaDataFor(aPlacesNode.uri);
        historyDataItem = new DownloadsHistoryDataItem(aPlacesNode);
        historyDataItem.updateFromMetaData(metaData);
      }
      let shell = new DownloadElementShell(aDataItem, historyDataItem);
      shell.element._placesNode = aPlacesNode;
      newOrUpdatedShell = shell;
      shellsForURI.add(shell);
      if (aDataItem) {
        this._viewItemsForDataItems.set(aDataItem, shell);
      }
    } else if (aPlacesNode) {
      
      
      
      
      
      
      
      
      
      
      
      for (let shell of shellsForURI) {
        if (!shell.historyDataItem) {
          
          shell.historyDataItem = new DownloadsHistoryDataItem(aPlacesNode);
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
      } else if (aDataItem) {
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
        if (shell.sessionDataItem) {
          shell.historyDataItem = null;
        } else {
          this._removeElement(shell.element);
          shellsForURI.delete(shell);
          if (shellsForURI.size == 0)
            this._downloadElementsShellsForURI.delete(downloadURI);
        }
      }
    }
  },

  _removeSessionDownloadFromView(aDataItem) {
    let shells = this._downloadElementsShellsForURI
                     .get(aDataItem.download.source.url);
    if (shells.size == 0) {
      throw new Error("Should have had at leaat one shell for this uri");
    }

    let shell = this._viewItemsForDataItems.get(aDataItem);
    if (!shells.has(shell)) {
      throw new Error("Missing download element shell in shells list for url");
    }

    
    
    
    
    if (shells.size > 1 || !shell.historyDataItem) {
      this._removeElement(shell.element);
      shells.delete(shell);
      if (shells.size == 0) {
        this._downloadElementsShellsForURI.delete(aDataItem.download.source.url);
      }
    } else {
      
      
      
      
      
      let url = shell.historyDataItem.download.source.url;
      let metaData = this._getPlacesMetaDataFor(url);
      shell.historyDataItem.updateFromMetaData(metaData);
      shell.sessionDataItem = null;
      
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

    
    
    
    
    this._cachedPlacesMetaData = null;

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

  onDataItemAdded(aDataItem, aNewest) {
    this._addDownloadData(aDataItem, null, aNewest);
  },

  onDataItemRemoved(aDataItem) {
    this._removeSessionDownloadFromView(aDataItem);
  },

  
  onDataItemStateChanged(aDataItem) {
    this._viewItemsForDataItems.get(aDataItem).onStateChanged();
  },

  
  onDataItemChanged(aDataItem) {
    this._viewItemsForDataItems.get(aDataItem).onChanged();
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
      if (!elt._shell.dataItem.inProgress) {
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
    let state = element._shell.dataItem.state;
    contextMenu.setAttribute("state", state);

    if (state == nsIDM.DOWNLOAD_DOWNLOADING) {
      
      
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
