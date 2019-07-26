









let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/DownloadUtils.jsm");
Cu.import("resource:///modules/DownloadsCommon.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
                                  "resource:///modules/RecentWindow.jsm");

const nsIDM = Ci.nsIDownloadManager;

const DESTINATION_FILE_URI_ANNO  = "downloads/destinationFileURI";
const DESTINATION_FILE_NAME_ANNO = "downloads/destinationFileName";
const DOWNLOAD_META_DATA_ANNO    = "downloads/metaData";

const DOWNLOAD_VIEW_SUPPORTED_COMMANDS =
 ["cmd_delete", "cmd_copy", "cmd_paste", "cmd_selectAll",
  "downloadsCmd_pauseResume", "downloadsCmd_cancel",
  "downloadsCmd_open", "downloadsCmd_show", "downloadsCmd_retry",
  "downloadsCmd_openReferrer", "downloadsCmd_clearDownloads"];

const NOT_AVAILABLE = Number.MAX_VALUE;

function GetFileForFileURI(aFileURI)
  Cc["@mozilla.org/network/protocol;1?name=file"]
    .getService(Ci.nsIFileProtocolHandler)
    .getFileFromURLSpec(aFileURI);






























function DownloadElementShell(aDataItem, aPlacesNode, aAnnotations) {
  this._element = document.createElement("richlistitem");
  this._element._shell = this;

  this._element.classList.add("download");
  this._element.classList.add("download-state");

  if (aAnnotations)
    this._annotations = aAnnotations;
  if (aDataItem)
    this.dataItem = aDataItem;
  if (aPlacesNode)
    this.placesNode = aPlacesNode;
}

DownloadElementShell.prototype = {
  
  get element() this._element,

  







  ensureActive: function DES_ensureActive() {
    if (this._active)
      return false;
    this._active = true;
    this._element.setAttribute("active", true);
    this._updateStatusUI();
    this._fetchTargetFileInfo();
    return true;
  },
  get active() !!this._active,

  
  _dataItem: null,
  get dataItem() this._dataItem,

  set dataItem(aValue) {
    this._dataItem = aValue;
    let shouldUpdate = false;
    if (this._dataItem) {
      this._invalidateMetaDataAndTargetFileInfo();
      
      
      shouldUpdate = !this.ensureActive();
    }
    else if (this._placesNode) {
      this._targetFileInfoFetched = false;
      shouldUpdate = this.active;
    }
    else {
      throw new Error("Should always have either a dataItem or a placesNode");
    }

    if (shouldUpdate) {
      this._fetchTargetFileInfo();
      this._updateStatusUI();
    }
    return aValue;
  },

  _placesNode: null,
  get placesNode() this._placesNode,
  set placesNode(aNode) {
    if (this._placesNode != aNode) {
      
      
      if (this._placesNode || !this._annotations) {
        this._annotations = new Map();
      }
      this._placesNode = aNode;

      
      
      if (!this._dataItem) {
        if (!this._placesNode)
          throw new Error("Should always have either a dataItem or a placesNode");
        this._invalidateMetaDataAndTargetFileInfo();
        if (this.active)
          this._updateStatusUI();
      }
    }
    return aNode;
  },

  
  get downloadURI() {
    if (this._dataItem)
     return this._dataItem.uri;
    if (this._placesNode)
      return this._placesNode.uri;
    throw new Error("Unexpected download element state");
  },

  get _downloadURIObj() {
    if (!("__downloadURIObj" in this))
      this.__downloadURIObj = NetUtil.newURI(this.downloadURI);
    return this.__downloadURIObj;
  },

  get _icon() {
    if (this._targetFileURI)
      return "moz-icon://" + this._targetFileURI + "?size=32";
    if (this._placesNode) {
      
      let ext = this._downloadURIObj.QueryInterface(Ci.nsIURL).fileExtension;
      if (ext)
        return "moz-icon://." + ext + "?size=32";
      return this._placesNode.icon || "moz-icon://.unknown?size=32";
    }
    if (this._dataItem)
      throw new Error("Session-download items should always have a target file uri");
    throw new Error("Unexpected download element state");
  },

  
  _getAnnotation: function DES__getAnnotation(aAnnotation, aDefaultValue) {
    let value;
    if (this._annotations.has(aAnnotation))
      value = this._annotations.get(aAnnotation);

    
    
    if (value === undefined) {
      try {
        value = PlacesUtils.annotations.getPageAnnotation(
          this._downloadURIObj, aAnnotation);
      }
      catch(ex) {
        value = NOT_AVAILABLE;
      }
    }

    if (value === NOT_AVAILABLE) {
      if (aDefaultValue === undefined) {
        throw new Error("Could not get required annotation '" + aAnnotation +
                        "' for download with url '" + this.downloadURI + "'");
      }
      value = aDefaultValue;
    }

    this._annotations.set(aAnnotation, value);
    return value;
  },

  
  get _displayName() {
    if (this._dataItem)
      return this._dataItem.target;

    try {
      return this._getAnnotation(DESTINATION_FILE_NAME_ANNO);
    }
    catch(ex) { }

    
    return this._placesNode.title || this.downloadURI;
  },

  
  get _targetFileURI() {
    if (this._dataItem)
      return this._dataItem.file;

    return this._getAnnotation(DESTINATION_FILE_URI_ANNO, "");
  },

  get _targetFilePath() {
    let fileURI = this._targetFileURI;
    if (fileURI)
      return GetFileForFileURI(fileURI).path;
    return "";
  },

  _fetchTargetFileInfo: function DES__fetchTargetFileInfo() {
    if (this._targetFileInfoFetched)
      throw new Error("_fetchTargetFileInfo should not be called if the information was already fetched");
    if (!this.active)
      throw new Error("Trying to _fetchTargetFileInfo on an inactive download shell");

    let path = this._targetFilePath;

    
    
    if (!path) {
      this._targetFileInfoFetched = true;
      this._targetFileExists = false;
      return;
    }

    OS.File.stat(path).then(
      function onSuccess(fileInfo) {
        this._targetFileInfoFetched = true;
        this._targetFileExists = true;
        this._targetFileSize = fileInfo.size;
        this._getDownloadMetaData().fileSize = this._targetFileSize;
        this._updateDownloadStatusUI();
      }.bind(this),

      function onFailure(aReason) {
        if (reason instanceof OS.File.Error && reason.becauseNoSuchFile) {
          this._targetFileInfoFetched = true;
          this._targetFileExists = false;
        }
        else {
          Cu.reportError("Could not fetch info for target file (reason: " +
                         aReason + ")");
        }

        this._updateDownloadStatusUI();
      }.bind(this)
    );
  },

  











  _getDownloadMetaData: function DES__getDownloadMetaData() {
    if (!this._metaData) {
      if (this._dataItem) {
        this._metaData = {
          state: this._dataItem.state,
          endTime: this._dataItem.endTime
        };
        if (this._dataItem.done)
          this._metaData.fileSize = this._dataItem.maxBytes;
      }
      else {
        try {
          this._metaData = JSON.parse(this._getAnnotation(DOWNLOAD_META_DATA_ANNO));
        }
        catch(ex) {
          this._metaData = { };
          if (this._targetFileInfoFetched && this._targetFileExists) {
            this._metaData.state = this._targetFileSize > 0 ?
              nsIDM.DOWNLOAD_FINISHED : nsIDM.DOWNLOAD_FAILED;
            this._metaData.fileSize = this._targetFileSize;
          }

          
          this._metaData.endTime = this._placesNode.time / 1000;
        }
      }
    }
    return this._metaData;
  },

  
  _getStatusText: function DES__getStatusText() {
    let s = DownloadsCommon.strings;
    if (this._dataItem && this._dataItem.inProgress) {
      if (this._dataItem.paused) {
        let transfer =
          DownloadUtils.getTransferTotal(this._dataItem.currBytes,
                                         this._dataItem.maxBytes);

         
         
         return s.statusSeparatorBeforeNumber(s.statePaused, transfer);
      }
      if (this._dataItem.state == nsIDM.DOWNLOAD_DOWNLOADING) {
        let [status, newEstimatedSecondsLeft] =
          DownloadUtils.getDownloadStatus(this.dataItem.currBytes,
                                          this.dataItem.maxBytes,
                                          this.dataItem.speed,
                                          this._lastEstimatedSecondsLeft);
        this._lastEstimatedSecondsLeft = newEstimatedSecondsLeft;
        return status;
      }
      if (this._dataItem.starting) {
        return s.stateStarting;
      }
      if (this._dataItem.state == nsIDM.DOWNLOAD_SCANNING) {
        return s.stateScanning;
      }

      throw new Error("_getStatusText called with a bogus download state");
    }

    
    let stateLabel = "";
    let state = this._getDownloadMetaData().state;
    switch (state) {
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
      case nsIDM.DOWNLOAD_FINISHED:{
        
        let metaData = this._getDownloadMetaData();
        if ("fileSize" in metaData) {
          let [size, unit] = DownloadUtils.convertByteUnits(metaData.fileSize);
          stateLabel = s.sizeWithUnits(size, unit);
          break;
        }
        
      }
      default:
        stateLabel = s.sizeUnknown;
        break;
    }

    
    let referrer = this._dataItem && this._dataItem.referrer ||
                   this.downloadURI;
    let [displayHost, fullHost] = DownloadUtils.getURIHost(referrer);

    let date = new Date(this._getDownloadMetaData().endTime);
    let [displayDate, fullDate] = DownloadUtils.getReadableDates(date);

    
    
    let firstPart = s.statusSeparator(stateLabel, displayHost);
    return s.statusSeparator(firstPart, displayDate);
  },

  
  get _progressElement() {
    if (!("__progressElement" in this)) {
      this.__progressElement =
        document.getAnonymousElementByAttribute(this._element, "anonid",
                                                "progressmeter");
    }
    return this.__progressElement;
  },

  
  
  
  _updateDownloadStatusUI: function  DES__updateDownloadStatusUI() {
    let state = this._getDownloadMetaData().state;
    if (state !== undefined)
      this._element.setAttribute("state", state);

    this._element.setAttribute("status", this._getStatusText());

    
    
    if (!this._dataItem)
      return;

    
    if (this._dataItem.starting) {
      
      this._element.setAttribute("progressmode", "normal");
      this._element.setAttribute("progress", "0");
    }
    else if (this._dataItem.state == nsIDM.DOWNLOAD_SCANNING ||
             this._dataItem.percentComplete == -1) {
      
      
      this._element.setAttribute("progressmode", "undetermined");
    }
    else {
      
      this._element.setAttribute("progressmode", "normal");
      this._element.setAttribute("progress", this._dataItem.percentComplete);
    }

    
    if (this._progressElement) {
      let event = document.createEvent("Events");
      event.initEvent("ValueChange", true, true);
      this._progressElement.dispatchEvent(event);
    }
  },

  _updateStatusUI: function DES__updateStatusUI() {
    if (!this.active)
      throw new Error("Trying to _updateStatusUI on an inactive download shell");
    this._element.setAttribute("displayName", this._displayName);
    this._element.setAttribute("image", this._icon);
    this._updateDownloadStatusUI();
  },

  placesNodeIconChanged: function DES_placesNodeIconChanged() {
    if (!this._dataItem)
      this._element.setAttribute("image", this._icon);
  },

  placesNodeTitleChanged: function DES_placesNodeTitleChanged() {
    if (!this._dataItem)
      this._element.setAttribute("displayName", this._displayName);
  },

  placesNodeAnnotationChanged: function DES_placesNodeAnnotationChanged(aAnnoName) {
    this._annotations.delete(aAnnoName);
    if (!this._dataItem) {
      if (aAnnoName == DESTINATION_FILE_URI_ANNO) {
        this._element.setAttribute("image", this._icon);
        this._updateDownloadStatusUI();
      }
      else if (aAnnoName == DESTINATION_FILE_NAME_ANNO) {
        this._element.setAttribute("displayName", this._displayName);
      }
      else if (aAnnoName == DOWNLOAD_META_DATA_ANNO) {
        this._invalidateMetaDataAndTargetFileInfo();
        this._updateDownloadStatusUI();
        if (this._element.selected)
          goUpdateDownloadCommands();
      }
    }
  },

  _invalidateMetaDataAndTargetFileInfo: function DES__invalidateMetaDataAndTargetFileInfo() {
    this._metaData = null;
    this._targetFileInfoFetched = false;
    if (this.active)
      this._fetchTargetFileInfo();
  },

  
  onStateChange: function DES_onStateChange(aOldState) {
    if (aOldState != nsIDM.DOWNLOAD_FINISHED &&
        aOldState != this.dataItem.state) {
      
      this._element.setAttribute("image", this._icon + "&state=normal");
      if (this.active)
        this._fetchTargetFileInfo();
    }

    this._metaData = null;
    this._updateDownloadStatusUI();
    if (this._element.selected)
      goUpdateDownloadCommands();
  },

  
  onProgressChange: function DES_onProgressChange() {
    this._updateDownloadStatusUI();
  },

  
  isCommandEnabled: function DES_isCommandEnabled(aCommand) {
    
    if (!this.active && aCommand != "cmd_delete")
      return false;
    switch (aCommand) {
      case "downloadsCmd_open": {
        
        
        
        if (this._dataItem && !this._dataItem.openable)
          return false;

        
        
        if (!this._targetFileInfoFetched)
          return false;

        return this._targetFileExists;
      }
      case "downloadsCmd_show": {
        
        if (this._dataItem &&
            this._dataItem.partFile && this._dataItem.partFile.exists())
          return true;

        
        
        if (!this._targetFileInfoFetched)
          return false;

        return this._targetFileExists;
      }
      case "downloadsCmd_pauseResume":
        return this._dataItem && this._dataItem.inProgress && this._dataItem.resumable;
      case "downloadsCmd_retry":
        
        return !this._dataItem || this._dataItem.canRetry;
      case "downloadsCmd_openReferrer":
        return this._dataItem && !!this._dataItem.referrer;
      case "cmd_delete":
        
        if (this._placesNode && this._dataItem && this._dataItem.inProgress)
          return false;
        return true;
      case "downloadsCmd_cancel":
        return this._dataItem != null;
    }
    return false;
  },

  _retryAsHistoryDownload: function DES__retryAsHistoryDownload() {
    
    
    

    
    
    let browserWin = RecentWindow.getMostRecentBrowserWindow();
    let initiatingDoc = browserWin ? browserWin.document : document;
    saveURL(this.downloadURI, this._displayName, null, true, true, undefined,
            initiatingDoc);
  },

  
  doCommand: function DES_doCommand(aCommand) {
    switch (aCommand) {
      case "downloadsCmd_open": {
        if (this._dateItem)
          this._dataItem.openLocalFile(window);
        else
          DownloadsCommon.openDownloadedFile(
            GetFileForFileURI(this._targetFileURI), null, window);
        break;
      }
      case "downloadsCmd_show": {
        if (this._dataItem)
          this._dataItem.showLocalFile();
        else
          DownloadsCommon.showDownloadedFile(
            GetFileForFileURI(this._targetFileURI));
        break;
      }
      case "downloadsCmd_openReferrer": {
        openURL(this._dataItem.referrer);
        break;
      }
      case "downloadsCmd_cancel": {
        this._dataItem.cancel();
        break;
      }
      case "cmd_delete": {
        if (this._dataItem)
          this._dataItem.remove();
        if (this._placesNode)
          PlacesUtils.bhistory.removePage(this._downloadURIObj);
        break;
       }
      case "downloadsCmd_retry": {
        if (this._dataItem)
          this._dataItem.retry();
        else
          this._retryAsHistoryDownload();
        break;
      }
      case "downloadsCmd_pauseResume": {
        this._dataItem.togglePauseResume();
        break;
      }
    }
  },

  
  
  
  matchesSearchTerm: function DES_matchesSearchTerm(aTerm) {
    if (!aTerm)
      return true;
    aTerm = aTerm.toLowerCase();
    return this._displayName.toLowerCase().indexOf(aTerm) != -1 ||
           this.downloadURI.toLowerCase().indexOf(aTerm) != -1;
  },

  
  
  doDefaultCommand: function DES_doDefaultCommand() {
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
        case nsIDM.DOWNLOAD_DOWNLOADING:
        case nsIDM.DOWNLOAD_SCANNING:
          return "downloadsCmd_show";
        case nsIDM.DOWNLOAD_BLOCKED_PARENTAL:
        case nsIDM.DOWNLOAD_DIRTY:
        case nsIDM.DOWNLOAD_BLOCKED_POLICY:
          return "downloadsCmd_openReferrer";
      }
      return "";
    }
    let command = getDefaultCommandForState(this._getDownloadMetaData().state);
    if (this.isCommandEnabled(command))
      this.doCommand(command);
  }
};













function DownloadsPlacesView(aRichListBox, aActive = true) {
  this._richlistbox = aRichListBox;
  this._richlistbox._placesView = this;
  this._richlistbox.controllers.appendController(this);

  
  this._downloadElementsShellsForURI = new Map();

  
  this._viewItemsForDataItems = new WeakMap();

  
  
  this._lastSessionDownloadElement = null;

  this._searchTerm = "";

  this._active = aActive;

  
  
  this._initiallySelectedElement = null;
  let downloadsData = DownloadsCommon.getData(window.opener || window);
  downloadsData.addView(this);

  
  window.addEventListener("unload", function() {
    this._richlistbox.controllers.removeController(this);
    downloadsData.removeView(this);
    this.result = null;
  }.bind(this), true);
  
  window.addEventListener("resize", function() {
    this._ensureVisibleElementsAreActive();
  }.bind(this), true);
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

  _forEachDownloadElementShellForURI:
  function DPV__forEachDownloadElementShellForURI(aURI, aCallback) {
    if (this._downloadElementsShellsForURI.has(aURI)) {
      let downloadElementShells = this._downloadElementsShellsForURI.get(aURI);
      for (let des of downloadElementShells) {
        aCallback(des);
      }
    }
  },

  _getAnnotationsFor: function DPV_getAnnotationsFor(aURI) {
    if (!this._cachedAnnotations) {
      this._cachedAnnotations = new Map();
      for (let name of [ DESTINATION_FILE_URI_ANNO,
                         DESTINATION_FILE_NAME_ANNO,
                         DOWNLOAD_META_DATA_ANNO ]) {
        let results = PlacesUtils.annotations.getAnnotationsWithName(name);
        for (let result of results) {
          let url = result.uri.spec;
          if (!this._cachedAnnotations.has(url))
            this._cachedAnnotations.set(url, new Map());
          let m = this._cachedAnnotations.get(url);
          m.set(result.annotationName, result.annotationValue);
        }
      }
    }

    let annotations = this._cachedAnnotations.get(aURI);
    if (!annotations) {
      
      
      annotations = new Map();
      annotations.set(DESTINATION_FILE_URI_ANNO, NOT_AVAILABLE);
      annotations.set(DESTINATION_FILE_NAME_ANNO, NOT_AVAILABLE);
    }
    
    if (!annotations.has(DOWNLOAD_META_DATA_ANNO)) {
      annotations.set(DOWNLOAD_META_DATA_ANNO, NOT_AVAILABLE);
    }
    return annotations;
  },

  


























  _addDownloadData:
  function DPV_addDownloadData(aDataItem, aPlacesNode, aNewest = false,
                               aDocumentFragment = null) {
    let downloadURI = aPlacesNode ? aPlacesNode.uri : aDataItem.uri;
    let shellsForURI = this._downloadElementsShellsForURI.get(downloadURI, null);
    if (!shellsForURI) {
      shellsForURI = new Set();
      this._downloadElementsShellsForURI.set(downloadURI, shellsForURI);
    }

    let newOrUpdatedShell = null;

    
    
    let shouldCreateShell = shellsForURI.size == 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (!shouldCreateShell &&
        aDataItem && this.getViewItem(aDataItem) == null) {
      
      
      
      shouldCreateShell = true;
      for (let shell of shellsForURI) {
        if (!shell.dataItem) {
          shouldCreateShell = false;
          shell.dataItem = aDataItem;
          newOrUpdatedShell = shell;
          this._viewItemsForDataItems.set(aDataItem, shell);
          break;
        }
      }
    }

    if (shouldCreateShell) {
      let shell = new DownloadElementShell(aDataItem, aPlacesNode,
                                           this._getAnnotationsFor(downloadURI));
      newOrUpdatedShell = shell;
      shellsForURI.add(shell);
      if (aDataItem)
        this._viewItemsForDataItems.set(aDataItem, shell);
    }
    else if (aPlacesNode) {
      for (let shell of shellsForURI) {
        if (shell.placesNode != aPlacesNode)
          shell.placesNode = aPlacesNode;
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
      }
      else if (aDataItem) {
        let before = this._lastSessionDownloadElement ?
          this._lastSessionDownloadElement.nextSibling : this._richlistbox.firstChild;
        this._richlistbox.insertBefore(newOrUpdatedShell.element, before);
        this._lastSessionDownloadElement = newOrUpdatedShell.element;
      }
      else {
        let appendTo = aDocumentFragment || this._richlistbox;
        appendTo.appendChild(newOrUpdatedShell.element);
      }

      if (this.searchTerm) {
        newOrUpdatedShell.element.hidden =
          !newOrUpdatedShell.element._shell.matchesSearchTerm(this.searchTerm);
      }
    }

    
    
    if (!aDocumentFragment)
      this._ensureVisibleElementsAreActive();
  },

  _removeElement: function DPV__removeElement(aElement) {
    
    
    if (aElement.nextSibling &&
        this._richlistbox.selectedItems &&
        this._richlistbox.selectedItems.length > 0 &&
        this._richlistbox.selectedItems[0] == aElement) {
      this._richlistbox.selectItem(aElement.nextSibling);
    }
    this._richlistbox.removeChild(aElement);
    this._ensureVisibleElementsAreActive();
  },

  _removeHistoryDownloadFromView:
  function DPV__removeHistoryDownloadFromView(aPlacesNode) {
    let downloadURI = aPlacesNode.uri;
    let shellsForURI = this._downloadElementsShellsForURI.get(downloadURI, null);
    if (shellsForURI) {
      for (let shell of shellsForURI) {
        if (shell.dataItem) {
          shell.placesNode = null;
        }
        else {
          this._removeElement(shell.element);
          shellsForURI.delete(shell);
          if (shellsForURI.size == 0)
            this._downloadElementsShellsForURI.delete(downloadURI);
        }
      }
    }
  },

  _removeSessionDownloadFromView:
  function DPV__removeSessionDownloadFromView(aDataItem) {
    let shells = this._downloadElementsShellsForURI.get(aDataItem.uri, null);
    if (shells.size == 0)
      throw new Error("Should have had at leaat one shell for this uri");

    let shell = this.getViewItem(aDataItem);
    if (!shells.has(shell))
      throw new Error("Missing download element shell in shells list for url");

    
    
    
    
    if (shells.size > 1 || !shell.placesNode) {
      this._removeElement(shell.element);
      shells.delete(shell);
      if (shells.size == 0)
        this._downloadElementsShellsForURI.delete(aDataItem.uri);
    }
    else {
      shell.dataItem = null;
      
      if (this._lastSessionDownloadElement == shell.dataItem) {
        this._lastSessionDownloadElement = shell.dataItem.previousSibling;
      }
      else {
        let before = this._lastSessionDownloadElement ?
          this._lastSessionDownloadElement.nextSibling : this._richlistbox.firstChild;
        this._richlistbox.insertBefore(shell.element, before);
      }
    }
  },

  _ensureVisibleElementsAreActive:
  function DPV__ensureVisibleElementsAreActive() {
    if (!this.active || this._ensureVisibleTimer || !this._richlistbox.firstChild)
      return;

    this._ensureVisibleTimer = setTimeout(function() {
      delete this._ensureVisibleTimer;
      if (!this._richlistbox.firstChild)
        return;

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
          
          if (!lastVisibleNode)
            lastVisibleNode = node;
        }
      }

      
      
      
      let nodeBelowVisibleArea = lastVisibleNode && lastVisibleNode.nextSibling;
      if (nodeBelowVisibleArea && nodeBelowVisibleArea._shell)
        nodeBelowVisibleArea._shell.ensureActive();

      let nodeABoveVisibleArea =
        firstVisibleNode && firstVisibleNode.previousSibling;
      if (nodeABoveVisibleArea && nodeABoveVisibleArea._shell)
        nodeABoveVisibleArea._shell.ensureActive();
    }.bind(this), 10);
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
    if (!queries.value.length)
      queries.value = [history.getNewQuery()];

    let result = history.executeQueries(queries.value, queries.value.length,
                                        options.value);
    result.addObserver(this, false);
    return val;
  },

  _result: null,
  get result() this._result,
  set result(val) {
    if (this._result == val)
      return val;

    if (this._result) {
      this._result.removeObserver(this);
      this._resultNode.containerOpen = false;
    }

    if (val) {
      this._result = val;
      this._resultNode = val.root;
      this._resultNode.containerOpen = true;
      this._ensureInitialSelection();
    }
    else {
      delete this._resultNode;
      delete this._result;
    }

    return val;
  },

  get selectedNodes() {
    let placesNodes = [];
    let selectedElements = this._richlistbox.selectedItems;
    for (let elt of selectedElements) {
      if (elt._shell.placesNode)
        placesNodes.push(elt._shell.placesNode);
    }
    return placesNodes;
  },

  get selectedNode() {
    let selectedNodes = this.selectedNodes;
    return selectedNodes.length == 1 ? selectedNodes[0] : null;
  },

  get hasSelection() this.selectedNodes.length > 0,

  containerStateChanged:
  function DPV_containerStateChanged(aNode, aOldState, aNewState) {
    this.invalidateContainer(aNode)
  },

  invalidateContainer:
  function DPV_invalidateContainer(aContainer) {
    if (aContainer != this._resultNode)
      throw new Error("Unexpected container node");
    if (!aContainer.containerOpen)
      throw new Error("Root container for the downloads query cannot be closed");

    
    
    for (let element of this._richlistbox.childNodes) {
      if (element._shell.placesNode)
        this._removeHistoryDownloadFromView(element._shell.placesNode);
    }

    let elementsToAppendFragment = document.createDocumentFragment();
    for (let i = 0; i < aContainer.childCount; i++) {
      try {
        this._addDownloadData(null, aContainer.getChild(i), false,
                              elementsToAppendFragment);
      }
      catch(ex) {
        Cu.reportError(ex);
      }
    }

    this._appendDownloadsFragment(elementsToAppendFragment);
    this._ensureVisibleElementsAreActive();
  },

  _appendDownloadsFragment: function DPV__appendDownloadsFragment(aDOMFragment) {
    
    
    let parentNode = this._richlistbox.parentNode;
    let nextSibling = this._richlistbox.nextSibling;
    this._richlistbox.controllers.removeController(this);
    parentNode.removeChild(this._richlistbox);
    this._richlistbox.appendChild(aDOMFragment);
    parentNode.insertBefore(this._richlistbox, nextSibling);
    this._richlistbox.controllers.appendController(this);
  },

  nodeInserted: function DPV_nodeInserted(aParent, aPlacesNode) {
    this._addDownloadData(null, aPlacesNode);
  },

  nodeRemoved: function DPV_nodeRemoved(aParent, aPlacesNode, aOldIndex) {
    this._removeHistoryDownloadFromView(aPlacesNode);
  },

  nodeIconChanged: function DPV_nodeIconChanged(aNode) {
    this._forEachDownloadElementShellForURI(aNode.uri, function(aDownloadElementShell) {
      aDownloadElementShell.placesNodeIconChanged();
    });
  },

  nodeAnnotationChanged: function DPV_nodeAnnotationChanged(aNode, aAnnoName) {
    this._forEachDownloadElementShellForURI(aNode.uri, function(aDownloadElementShell) {
      aDownloadElementShell.placesNodeAnnotationChanged(aAnnoName);
    });
  },

  nodeTitleChanged: function DPV_nodeTitleChanged(aNode, aNewTitle) {
    this._forEachDownloadElementShellForURI(aNode.uri, function(aDownloadElementShell) {
      aDownloadElementShell.placesNodeTitleChanged();
    });
  },

  nodeKeywordChanged: function() {},
  nodeDateAddedChanged: function() {},
  nodeLastModifiedChanged: function() {},
  nodeReplaced: function() {},
  nodeHistoryDetailsChanged: function() {},
  nodeTagsChanged: function() {},
  sortingChanged: function() {},
  nodeMoved: function() {},
  nodeURIChanged: function() {},
  batching: function() {},

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

  














  _ensureInitialSelection: function DPV__ensureInitialSelection() {
    
    if (this._richlistbox.selectedItem == this._initiallySelectedElement) {
      let firstDownloadElement = this._richlistbox.firstChild;
      if (firstDownloadElement != this._initiallySelectedElement) {
        
        
        
        
        firstDownloadElement._shell.ensureActive();
        Services.tm.mainThread.dispatch(function() {
          this._richlistbox.selectedItem = firstDownloadElement;
          this._richlistbox.currentItem = firstDownloadElement;
          this._initiallySelectedElement = firstDownloadElement;
        }.bind(this), Ci.nsIThread.DISPATCH_NORMAL);
      }
    }
  },

  onDataLoadStarting: function() { },
  onDataLoadCompleted: function DPV_onDataLoadCompleted() {
    this._ensureInitialSelection();
  },

  onDataItemAdded: function DPV_onDataItemAdded(aDataItem, aNewest) {
    this._addDownloadData(aDataItem, null, aNewest);
  },

  onDataItemRemoved: function DPV_onDataItemRemoved(aDataItem) {
    this._removeSessionDownloadFromView(aDataItem);
  },

  getViewItem: function(aDataItem)
    this._viewItemsForDataItems.get(aDataItem, null),

  supportsCommand: function(aCommand)
    DOWNLOAD_VIEW_SUPPORTED_COMMANDS.indexOf(aCommand) != -1,

  isCommandEnabled: function DPV_isCommandEnabled(aCommand) {
    let selectedElements = this._richlistbox.selectedItems;
    switch (aCommand) {
      case "cmd_copy":
        return selectedElements && selectedElements.length > 0;
      case "cmd_selectAll":
        return true;
      case "cmd_paste":
        return this._canDownloadClipboardURL();
      case "downloadsCmd_clearDownloads":
        return !!this._richlistbox.firstChild;
      default:
        return Array.every(selectedElements, function(element) {
          return element._shell.isCommandEnabled(aCommand);
        });
    }
  },

  _copySelectedDownloadsToClipboard:
  function DPV__copySelectedDownloadsToClipboard() {
    let selectedElements = this._richlistbox.selectedItems;
    let urls = [e._shell.downloadURI for each (e in selectedElements)];

    Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper).copyString(urls.join("\n"), document);
  },

  _getURLFromClipboardData: function DPV__getURLFromClipboardData() {
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
      if (url)
        return [NetUtil.newURI(url, null, null).spec, name];
    }
    catch(ex) { }

    return ["", ""];
  },

  _canDownloadClipboardURL: function DPV__canDownloadClipboardURL() {
    let [url, name] = this._getURLFromClipboardData();
    return url != "";
  },

  _downloadURLFromClipboard: function DPV__downloadURLFromClipboard() {
    let [url, name] = this._getURLFromClipboardData();
    saveURL(url, name || url, null, true, true, undefined, document);
  },

  doCommand: function DPV_doCommand(aCommand) {
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
        if (PrivateBrowsingUtils.isWindowPrivate(window)) {
          Services.downloads.cleanUpPrivate();
        } else {
          Services.downloads.cleanUp();
        }
        if (this.result) {
          Cc["@mozilla.org/browser/download-history;1"]
            .getService(Ci.nsIDownloadHistory)
            .removeAllDownloads();
        }
        break;
      default: {
        let selectedElements = this._richlistbox.selectedItems;
        for (let element of selectedElements) {
          element._shell.doCommand(aCommand);
        }
      }
    }
  },

  onEvent: function() { },

  onContextMenu: function DPV_onContextMenu(aEvent)
  {
    let element = this._richlistbox.selectedItem;
    if (!element || !element._shell)
      return false;

    
    let contextMenu = document.getElementById("downloadsContextMenu");
    let state = element._shell._getDownloadMetaData().state;
    if (state !== undefined)
      contextMenu.setAttribute("state", state);
    else
      contextMenu.removeAttribute("state");

    return true;
  },

  onKeyPress: function DPV_onKeyPress(aEvent) {
    let selectedElements = this._richlistbox.selectedItems;
    if (!selectedElements)
      return;

    if (aEvent.keyCode == KeyEvent.DOM_VK_RETURN) {
      
      
      
      if (selectedElements.length == 1) {
        let element = selectedElements[0];
        if (element._shell)
          element._shell.doDefaultCommand();
      }
    }
    else if (aEvent.charCode == " ".charCodeAt(0)) {
      
      for (let element of selectedElements) {
        if (element._shell.isCommandEnabled("downloadsCmd_pauseResume"))
          element._shell.doCommand("downloadsCmd_pauseResume");
      }
    }
  },

  onDoubleClick: function DPV_onDoubleClick(aEvent) {
    if (aEvent.button != 0)
      return;

    let selectedElements = this._richlistbox.selectedItems;
    if (!selectedElements || selectedElements.length != 1)
      return;

    let element = selectedElements[0];
    if (element._shell)
      element._shell.doDefaultCommand();
  },

  onScroll: function DPV_onScroll() {
    this._ensureVisibleElementsAreActive();
  }
};

for (let methodName of ["load", "applyFilter", "selectNode", "selectItems"]) {
  DownloadsPlacesView.prototype[methodName] = function() {
    throw new Error("|" + methodName + "| is not implemented by the downloads view.");
  }
}

function goUpdateDownloadCommands() {
  for (let command of DOWNLOAD_VIEW_SUPPORTED_COMMANDS) {
    goUpdateCommand(command);
  }
}
