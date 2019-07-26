









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

const nsIDM = Ci.nsIDownloadManager;

const DESTINATION_FILE_URI_ANNO  = "downloads/destinationFileURI";
const DESTINATION_FILE_NAME_ANNO = "downloads/destinationFileName";
const DOWNLOAD_STATE_ANNO        = "downloads/state";

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

  
  _dataItem: null,
  get dataItem() this._dataItem,

  set dataItem(aValue) {
    if ((this._dataItem = aValue)) {
      this._wasDone = this._dataItem.done;
      this._wasInProgress = this._dataItem.inProgress;
      this._targetFileInfoFetched = false;
      this._fetchTargetFileInfo();
    }
    else if (this._placesNode) {
      this._wasInProgress = false;
      this._wasDone = this.getDownloadState(true) == nsIDM.DOWNLOAD_FINISHED;
      this._targetFileInfoFetched = false;
      this._fetchTargetFileInfo();
    }

    this._updateStatusUI();
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

      
      
      if (!this._dataItem && this._placesNode) {
        this._wasInProgress = false;
        this._wasDone = this.getDownloadState(true) == nsIDM.DOWNLOAD_FINISHED;
        this._targetFileInfoFetched = false;
        this._updateStatusUI();
        this._fetchTargetFileInfo();
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
    if (this._placesNode)
      return this.placesNode.icon;
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
        delete this._state;
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

  










  getDownloadState: function DES_getDownloadState(aForceUpdate = false) {
    if (aForceUpdate || !("_state" in this)) {
      if (this._dataItem) {
        this._state = this._dataItem.state;
      }
      else {
        try {
          this._state = this._getAnnotation(DOWNLOAD_STATE_ANNO);
        }
        catch (ex) {
          if (!this._targetFileInfoFetched || !this._targetFileExists)
            this._state = undefined;
          else if (this._targetFileSize > 0)
            this._state = nsIDM.DOWNLOAD_FINISHED;
          else
            this._state = nsIDM.DOWNLOAD_FAILED;
        }
      }
    }
    return this._state;
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

      let [displayHost, fullHost] =
        DownloadUtils.getURIHost(this._dataItem.referrer ||
                                 this._dataItem.uri);

      let end = new Date(this.dataItem.endTime);
      let [displayDate, fullDate] = DownloadUtils.getReadableDates(end);
      return s.statusSeparator(fullHost, fullDate);
    }

    switch (this.getDownloadState()) {
      case nsIDM.DOWNLOAD_FAILED:
        return s.stateFailed;
      case nsIDM.DOWNLOAD_CANCELED:
        return s.stateCanceled;
      case nsIDM.DOWNLOAD_BLOCKED_PARENTAL:
        return s.stateBlockedParentalControls;
      case nsIDM.DOWNLOAD_BLOCKED_POLICY:
        return s.stateBlockedPolicy;
      case nsIDM.DOWNLOAD_DIRTY:
        return s.stateDirty;
      case nsIDM.DOWNLOAD_FINISHED:{
        
        if (this._targetFileInfoFetched && this._targetFileExists) {
          let [size, unit] = DownloadUtils.convertByteUnits(this._targetFileSize);
          return s.sizeWithUnits(size, unit);
        }
        break;
      }
    }

    return s.sizeUnknown;
  },

  
  get _progressElement() {
    let progressElement = document.getAnonymousElementByAttribute(
      this._element, "anonid", "progressmeter");
    if (progressElement) {
      delete this._progressElement;
      return this._progressElement = progressElement;
    }
    return null;
  },

  
  
  
  _updateDownloadStatusUI: function  DES__updateDownloadStatusUI() {
    let state = this.getDownloadState(true);
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
      else if (aAnnoName == DOWNLOAD_STATE_ANNO) {
        this._updateDownloadStatusUI();
        if (this._element.selected)
          goUpdateDownloadCommands();
      }
    }
  },

  
  onStateChange: function DES_onStateChange() {
    if (!this._wasDone && this._dataItem.done) {
      
      this._element.setAttribute("image", this._icon + "&state=normal");

      this._targetFileInfoFetched = false;
      this._fetchTargetFileInfo();
    }

    this._wasDone = this._dataItem.done;

    
    if (this._wasInProgress && !this._dataItem.inProgress) {
      this._endTime = Date.now();
    }

    this._wasDone = this._dataItem.done;
    this._wasInProgress = this._dataItem.inProgress;

    this._updateDownloadStatusUI();
    if (this._element.selected)
      goUpdateDownloadCommands();
  },

  
  onProgressChange: function DES_onProgressChange() {
    this._updateDownloadStatusUI();
  },

  
  isCommandEnabled: function DES_isCommandEnabled(aCommand) {
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
        
        return this._dataItem && this._dataItem.canRetry;
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
    
    saveURL(this.downloadURI, this._displayName, null, true, true, undefined, document);
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
    let command = getDefaultCommandForState(this.getDownloadState());
    if (this.isCommandEnabled(command))
      this.doCommand(command);
  }
};













function DownloadsPlacesView(aRichListBox) {
  this._richlistbox = aRichListBox;
  this._richlistbox._placesView = this;
  this._richlistbox.controllers.appendController(this);

  
  this._downloadElementsShellsForURI = new Map();

  
  this._viewItemsForDataItems = new WeakMap();

  
  
  this._lastSessionDownloadElement = null;

  this._searchTerm = "";

  
  
  let downloadsData = DownloadsCommon.getData(window.opener || window);
  downloadsData.addView(this);

  
  window.addEventListener("unload", function() {
    this._richlistbox.controllers.removeController(this);
    downloadsData.removeView(this);
    this.result = null;
  }.bind(this), true);
}

DownloadsPlacesView.prototype = {
  get associatedElement() this._richlistbox,

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
                         DOWNLOAD_STATE_ANNO ]) {
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
    
    if (!annotations.has(DOWNLOAD_STATE_ANNO)) {
      annotations.set(DOWNLOAD_STATE_ANNO, NOT_AVAILABLE);
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
  },

  _removeElement: function DPV__removeElement(aElement) {
    
    
    if (aElement.nextSibling &&
        this._richlistbox.selectedItems &&
        this._richlistbox.selectedItems.length > 0 &&
        this._richlistbox.selectedItems[0] == aElement) {
      this._richlistbox.selectItem(aElement.nextSibling);
    }
    this._richlistbox.removeChild(aElement);
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
      return;
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

    this._richlistbox.appendChild(elementsToAppendFragment);
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
    }
    return this._searchTerm = aValue;
  },

  applyFilter: function() {
    throw new Error("applyFilter is not implemented by the DownloadsView")
  },

  load: function(aQueries, aOptions) {
    throw new Error("|load| is not implemented by the Downloads View");
  },

  onDataLoadStarting: function() { },
  onDataLoadCompleted: function() { },

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
    let state = element._shell.getDownloadState();
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
  }
};

function goUpdateDownloadCommands() {
  for (let command of DOWNLOAD_VIEW_SUPPORTED_COMMANDS) {
    goUpdateCommand(command);
  }
}
