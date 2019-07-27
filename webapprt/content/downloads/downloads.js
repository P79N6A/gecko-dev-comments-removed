



"use strict";

const { interfaces: Ci, utils: Cu, classes: Cc } = Components;

const nsIDM = Ci.nsIDownloadManager;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/osfile.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DownloadUtils",
  "resource://gre/modules/DownloadUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
  "resource://gre/modules/PluralForm.jsm");

let gStr = {};

let DownloadItem = function(aID, aDownload) {
  this.id = aID;
  this._download = aDownload;

  this.file = aDownload.target.path;
  this.target = OS.Path.basename(aDownload.target.path);
  this.uri = aDownload.source.url;
  this.endTime = new Date();

  this.updateState();

  this.updateData();

  this.createItem();
};

DownloadItem.prototype = {
  


  element: null,

  


  _progressElement: null,

  _lastEstimatedSecondsLeft: Infinity,

  updateState: function() {
    
    if (this._download.succeeded) {
      this.state = nsIDM.DOWNLOAD_FINISHED;
    } else if (this._download.error &&
               this._download.error.becauseBlockedByParentalControls) {
      this.state = nsIDM.DOWNLOAD_BLOCKED_PARENTAL;
    } else if (this._download.error &&
               this._download.error.becauseBlockedByReputationCheck) {
      this.state = nsIDM.DOWNLOAD_DIRTY;
    } else if (this._download.error) {
      this.state = nsIDM.DOWNLOAD_FAILED;
    } else if (this._download.canceled && this._download.hasPartialData) {
      this.state = nsIDM.DOWNLOAD_PAUSED;
    } else if (this._download.canceled) {
      this.state = nsIDM.DOWNLOAD_CANCELED;
    } else if (this._download.stopped) {
      this.state = nsIDM.DOWNLOAD_NOTSTARTED;
    } else {
      this.state = nsIDM.DOWNLOAD_DOWNLOADING;
    }
  },

  updateData: function() {
    let wasInProgress = this.inProgress;

    this.updateState();

    if (wasInProgress && !this.inProgress) {
      this.endTime = new Date();
    }

    this.referrer = this._download.source.referrer;
    this.startTime = this._download.startTime;
    this.currBytes = this._download.currentBytes;
    this.resumable = this._download.hasPartialData;
    this.speed = this._download.speed;

    if (this._download.succeeded) {
      
      
      
      this.maxBytes = this._download.hasProgress ? this._download.totalBytes
                                                 : this._download.currentBytes;
      this.percentComplete = 100;
    } else if (this._download.hasProgress) {
      
      this.maxBytes = this._download.totalBytes;
      this.percentComplete = this._download.progress;
    } else {
      
      this.maxBytes = -1;
      this.percentComplete = -1;
    }
  },

  




  get inProgress() {
    return [
      nsIDM.DOWNLOAD_NOTSTARTED,
      nsIDM.DOWNLOAD_QUEUED,
      nsIDM.DOWNLOAD_DOWNLOADING,
      nsIDM.DOWNLOAD_PAUSED,
      nsIDM.DOWNLOAD_SCANNING,
    ].indexOf(this.state) != -1;
  },

  



  get starting() {
    return this.state == nsIDM.DOWNLOAD_NOTSTARTED ||
           this.state == nsIDM.DOWNLOAD_QUEUED;
  },

  


  get paused() {
    return this.state == nsIDM.DOWNLOAD_PAUSED;
  },

  



  get done() {
    return [
      nsIDM.DOWNLOAD_FINISHED,
      nsIDM.DOWNLOAD_BLOCKED_PARENTAL,
      nsIDM.DOWNLOAD_BLOCKED_POLICY,
      nsIDM.DOWNLOAD_DIRTY,
    ].indexOf(this.state) != -1;
  },

  


  get removable() {
    return [
      nsIDM.DOWNLOAD_FINISHED,
      nsIDM.DOWNLOAD_CANCELED,
      nsIDM.DOWNLOAD_BLOCKED_PARENTAL,
      nsIDM.DOWNLOAD_BLOCKED_POLICY,
      nsIDM.DOWNLOAD_DIRTY,
      nsIDM.DOWNLOAD_FAILED,
    ].indexOf(this.state) != -1;
  },

  


  get openable() {
    return this.state == nsIDM.DOWNLOAD_FINISHED;
  },

  



  get canRetry() {
    return this.state == nsIDM.DOWNLOAD_CANCELED ||
           this.state == nsIDM.DOWNLOAD_FAILED;
  },

  






  get localFile() {
    return this._getFile(this.file);
  },

  






  get partFile() {
    return this._getFile(this.file + ".part");
  },

  











  _getFile: function(aFilename) {
    
    
    
    if (aFilename.startsWith("file:")) {
      
      
      let fileUrl = NetUtil.newURI(aFilename).QueryInterface(Ci.nsIFileURL);
      return fileUrl.file;
    }

    
    
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    file.initWithPath(aFilename);
    return file;
  },

  


  showLocalFile: function() {
    let file = this.localFile;

    try {
      
      file.reveal();
    } catch (ex) {
      
      
      let parent = file.parent;
      if (!parent) {
        return;
      }

      try {
        
        parent.launch();
      } catch (ex) {
        
        
        Cc["@mozilla.org/uriloader/external-protocol-service;1"].
        getService(Ci.nsIExternalProtocolService).
        loadUrl(NetUtil.newURI(parent));
      }
    }
  },

  


  openLocalFile: function() {
    return this._download.launch();
  },

  


  pause: function() {
    return this._download.cancel();
  },

  


  resume: function() {
    return this._download.start();
  },

  


  retry: function() {
    return this._download.start();
  },

  


  cancel: function() {
    this._download.cancel();
    return this._download.removePartialData();
  },

  


  remove: function() {
    return Downloads.getList(Downloads.ALL)
                    .then(list => list.remove(this._download))
                    .then(() => this._download.finalize(true));
  },

  


  createItem: function() {
    this.element = document.createElement("richlistitem");

    this.element.setAttribute("id", this.id);
    this.element.setAttribute("target", this.target);
    this.element.setAttribute("state", this.state);
    this.element.setAttribute("progress", this.percentComplete);
    this.element.setAttribute("type", "download");
    this.element.setAttribute("image", "moz-icon://" + this.file + "?size=32");
  },

  


  updateStatusText: function() {
    let status = "";
    let statusTip = "";

    switch (this.state) {
      case nsIDM.DOWNLOAD_PAUSED:
        let transfer = DownloadUtils.getTransferTotal(this.currBytes,
                                                      this.maxBytes);
        status = gStr.paused.replace("#1", transfer);

        break;

      case nsIDM.DOWNLOAD_DOWNLOADING:
        let newLast;
        [status, newLast] =
          DownloadUtils.getDownloadStatus(this.currBytes, this.maxBytes,
                                          this.speed,
                                          this._lastEstimatedSecondsLeft);

        this._lastEstimatedSecondsLeft = newLast;

        break;

      case nsIDM.DOWNLOAD_FINISHED:
      case nsIDM.DOWNLOAD_FAILED:
      case nsIDM.DOWNLOAD_CANCELED:
      case nsIDM.DOWNLOAD_BLOCKED_PARENTAL:
      case nsIDM.DOWNLOAD_BLOCKED_POLICY:
      case nsIDM.DOWNLOAD_DIRTY:
        let stateSize = {};
        stateSize[nsIDM.DOWNLOAD_FINISHED] = () => {
          
          let sizeText = gStr.doneSizeUnknown;
          if (this.maxBytes >= 0) {
            let [size, unit] = DownloadUtils.convertByteUnits(this.maxBytes);
            sizeText = gStr.doneSize.replace("#1", size);
            sizeText = sizeText.replace("#2", unit);
          }
          return sizeText;
        };
        stateSize[nsIDM.DOWNLOAD_FAILED] = () => gStr.stateFailed;
        stateSize[nsIDM.DOWNLOAD_CANCELED] = () => gStr.stateCanceled;
        stateSize[nsIDM.DOWNLOAD_BLOCKED_PARENTAL] = () => gStr.stateBlocked;
        stateSize[nsIDM.DOWNLOAD_BLOCKED_POLICY] = () => gStr.stateBlockedPolicy;
        stateSize[nsIDM.DOWNLOAD_DIRTY] = () => gStr.stateDirty;

        
        status = gStr.doneStatus.replace("#1", stateSize[this.state]());

        let [displayHost, fullHost] =
          DownloadUtils.getURIHost(this.referrer || this.uri);

        
        status = status.replace("#2", displayHost);
        
        statusTip = fullHost;

        break;
    }

    this.element.setAttribute("status", status);
    this.element.setAttribute("statusTip", statusTip || status);
  },

  updateView: function() {
    this.updateData();

    
    if (this.starting) {
      
      this.element.setAttribute("progressmode", "normal");
      this.element.setAttribute("progress", "0");
    } else if (this.state == Ci.nsIDownloadManager.DOWNLOAD_SCANNING ||
               this.percentComplete == -1) {
      
      
      this.element.setAttribute("progressmode", "undetermined");
    } else {
      
      this.element.setAttribute("progressmode", "normal");
      this.element.setAttribute("progress", this.percentComplete);
    }

    
    if (!this._progressElement) {
      this._progressElement =
           document.getAnonymousElementByAttribute(this.element, "anonid",
                                                   "progressmeter");
    }

    
    if (this._progressElement) {
      let event = document.createEvent("Events");
      event.initEvent("ValueChange", true, true);
      this._progressElement.dispatchEvent(event);
    }

    
    

    
    
    if (!this.inProgress) {
      let [dateCompact, dateComplete] = DownloadUtils.getReadableDates(this.endTime);
      this.element.setAttribute("dateTime", dateCompact);
      this.element.setAttribute("dateTimeTip", dateComplete);
    }

    this.element.setAttribute("state", this.state);

    this.updateStatusText();

    
    let buttons = this.element.buttons;
    for (let i = 0; i < buttons.length; ++i) {
      let cmd = buttons[i].getAttribute("cmd");
      let enabled = this.isCommandEnabled(cmd);
      buttons[i].disabled = !enabled;

      if ("cmd_pause" == cmd && !enabled) {
        
        
        buttons[i].setAttribute("tooltiptext", gStr.cannotPause);
      }
    }

    if (this.done) {
      
      try {
        
        let mimeService = Cc["@mozilla.org/mime;1"].
                          getService(Ci.nsIMIMEService);
        let contentType = mimeService.getTypeFromFile(this.localFile);

        let oldImage = this.element.getAttribute("image");
        
        this.element.setAttribute("image", oldImage + "&contentType=" + contentType);
      } catch (e) {}
    }
  },

  






  matchesSearch: function(aTerms, aAttributes) {
    return aTerms.some(term => aAttributes.some(attr => this.element.getAttribute(attr).includes(term)));
  },

  isCommandEnabled: function(aCommand) {
    switch (aCommand) {
      case "cmd_cancel":
        return this.inProgress;

      case "cmd_open":
        return this.openable && this.localFile.exists();

      case "cmd_show":
        return this.localFile.exists();

      case "cmd_pause":
        return this.inProgress && !this.paused && this.resumable;

      case "cmd_pauseResume":
        return (this.inProgress || this.paused) && this.resumable;

      case "cmd_resume":
        return this.paused && this.resumable;

      case "cmd_openReferrer":
        return !!this.referrer;

      case "cmd_removeFromList":
        return this.removable;

      case "cmd_retry":
        return this.canRetry;

      case "cmd_copyLocation":
        return true;
    }

    return false;
  },

  doCommand: function(aCommand) {
    if (!this.isCommandEnabled(aCommand)) {
      return;
    }

    switch (aCommand) {
      case "cmd_cancel":
        this.cancel().catch(Cu.reportError);
        break;

      case "cmd_open":
        this.openLocalFile().catch(Cu.reportError);
        break;

      case "cmd_show":
        this.showLocalFile();
        break;

      case "cmd_pause":
        this.pause().catch(Cu.reportError);
        break;

      case "cmd_pauseResume":
        if (this.paused) {
          this.resume().catch(Cu.reportError);
        } else {
          this.pause().catch(Cu.reportError);
        }
        break;

      case "cmd_resume":
        this.resume().catch(Cu.reportError);
        break;

      case "cmd_openReferrer":
        let uri = Services.io.newURI(this.referrer || this.uri, null, null);

        
        Cc["@mozilla.org/uriloader/external-protocol-service;1"].
        getService(Ci.nsIExternalProtocolService).
        getProtocolHandlerInfo(uri.scheme).
        launchWithURI(uri);
        break;

      case "cmd_removeFromList":
        this.remove().catch(Cu.reportError);
        break;

      case "cmd_retry":
        this.retry().catch(Cu.reportError);
        break;

      case "cmd_copyLocation":
        let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].
                        getService(Ci.nsIClipboardHelper);
        clipboard.copyString(this.uri, document);
        break;
    }
  },
};

let gDownloadList = {
  downloadItemsMap: new Map(),
  idToDownloadItemMap: new Map(),
  _autoIncrementID: 0,
  downloadView: null,
  searchBox: null,
  searchTerms: [],
  searchAttributes: [ "target", "status", "dateTime", ],
  contextMenus: [
    
    [
      "menuitem_pause",
      "menuitem_cancel",
      "menuseparator",
      "menuitem_show",
      "menuseparator",
      "menuitem_openReferrer",
      "menuitem_copyLocation",
      "menuseparator",
      "menuitem_selectAll",
    ],
    
    [
      "menuitem_open",
      "menuitem_show",
      "menuseparator",
      "menuitem_openReferrer",
      "menuitem_copyLocation",
      "menuseparator",
      "menuitem_selectAll",
      "menuseparator",
      "menuitem_removeFromList"
    ],
    
    [
      "menuitem_retry",
      "menuseparator",
      "menuitem_openReferrer",
      "menuitem_copyLocation",
      "menuseparator",
      "menuitem_selectAll",
      "menuseparator",
      "menuitem_removeFromList",
    ],
    
    [
      "menuitem_retry",
      "menuseparator",
      "menuitem_openReferrer",
      "menuitem_copyLocation",
      "menuseparator",
      "menuitem_selectAll",
      "menuseparator",
      "menuitem_removeFromList",
    ],
    
    [
      "menuitem_resume",
      "menuitem_cancel",
      "menuseparator",
      "menuitem_show",
      "menuseparator",
      "menuitem_openReferrer",
      "menuitem_copyLocation",
      "menuseparator",
      "menuitem_selectAll",
    ],
    
    [
      "menuitem_cancel",
      "menuseparator",
      "menuitem_show",
      "menuseparator",
      "menuitem_openReferrer",
      "menuitem_copyLocation",
      "menuseparator",
      "menuitem_selectAll",
    ],
    
    [
      "menuitem_openReferrer",
      "menuitem_copyLocation",
      "menuseparator",
      "menuitem_selectAll",
      "menuseparator",
      "menuitem_removeFromList",
    ],
    
    [
      "menuitem_show",
      "menuseparator",
      "menuitem_openReferrer",
      "menuitem_copyLocation",
      "menuseparator",
      "menuitem_selectAll",
    ],
    
    [
      "menuitem_openReferrer",
      "menuitem_copyLocation",
      "menuseparator",
      "menuitem_selectAll",
      "menuseparator",
      "menuitem_removeFromList",
    ],
    
    [
      "menuitem_openReferrer",
      "menuitem_copyLocation",
      "menuseparator",
      "menuitem_selectAll",
      "menuseparator",
      "menuitem_removeFromList",
    ]
  ],

  init: function() {
    this.downloadView = document.getElementById("downloadView");
    this.searchBox = document.getElementById("searchbox");

    this.buildList(true);

    this.downloadView.focus();

    
    this.searchBox.addEventListener("keypress", (e) => {
      this.searchBoxKeyPressHandler(e);
    }, false);

    Downloads.getList(Downloads.ALL)
             .then(list => list.addView(this))
             .catch(Cu.reportError);
  },

  buildList: function(aForceBuild) {
    
    let prevSearch = this.searchTerms.join(" ");

    
    this.searchTerms = this.searchBox.value.trim().toLowerCase().split(/\s+/);

    
    if (!aForceBuild && this.searchTerms.join(" ") == prevSearch) {
      return;
    }

    
    let empty = this.downloadView.cloneNode(false);
    this.downloadView.parentNode.replaceChild(empty, this.downloadView);
    this.downloadView = empty;

    for (let downloadItem of this.idToDownloadItemMap.values()) {
      if (downloadItem.inProgress ||
          downloadItem.matchesSearch(this.searchTerms, this.searchAttributes)) {
        this.downloadView.appendChild(downloadItem.element);

        
        
        downloadItem.updateView();
      }
    }

    this.updateClearListButton();
  },

  


  clearList: Task.async(function*() {
    let searchTerms = this.searchTerms;

    let list = yield Downloads.getList(Downloads.ALL);

    yield list.removeFinished((aDownload) => {
      let downloadItem = this.downloadItemsMap.get(aDownload);
      if (!downloadItem) {
        Cu.reportError("Download doesn't exist.");
        return;
      }

      return downloadItem.matchesSearch(searchTerms, this.searchAttributes);
    });

    
    this.searchBox.value = "";
    this.searchBox.doCommand();
    this.downloadView.focus();
  }),

  



  updateClearListButton: function() {
    let button = document.getElementById("clearListButton");

    
    for (let downloadItem of this.idToDownloadItemMap.values()) {
      if (!downloadItem.inProgress &&
          downloadItem.matchesSearch(this.searchTerms, this.searchAttributes)) {
        button.disabled = false;
        return;
      }
    }

    button.disabled = true;
  },

  setSearchboxFocus: function() {
    this.searchBox.focus();
    this.searchBox.select();
  },

  searchBoxKeyPressHandler: function(aEvent) {
    if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE) {
      
      this.downloadView.focus();
      aEvent.preventDefault();
    }
  },

  buildContextMenu: function(aEvent) {
    if (aEvent.target.id != "downloadContextMenu") {
      return false;
    }

    let popup = document.getElementById("downloadContextMenu");
    while (popup.hasChildNodes()) {
      popup.removeChild(popup.firstChild);
    }

    if (this.downloadView.selectedItem) {
      let downloadItem = this._getSelectedDownloadItem();

      let idx = downloadItem.state;
      if (idx < 0) {
        idx = 0;
      }

      let menus = this.contextMenus[idx];
      for (let i = 0; i < menus.length; ++i) {
        let menuitem = document.getElementById(menus[i]).cloneNode(true);
        let cmd = menuitem.getAttribute("cmd");
        if (cmd) {
          menuitem.disabled = !downloadItem.isCommandEnabled(cmd);
        }

        popup.appendChild(menuitem);
      }

      return true;
    }

    return false;
  },

  


  doDefaultForSelected: function() {
    
    let download = this._getSelectedDownloadItem();
    if (!download) {
      return;
    }

    
    let menuitem = document.getElementById(this.contextMenus[download.state][0]);

    
    download.doCommand(menuitem.getAttribute("cmd"));
  },

  onDownloadDblClick: function(aEvent) {
    
    if (aEvent.button == 0 && aEvent.target.selected) {
      this.doDefaultForSelected();
    }
  },

  











  performCommand: function(aCmd, aItem) {
    if (!aItem) {
      
      let items = Array.from(this.downloadView.selectedItems);

      
      for (let item of items) {
        this.performCommand(aCmd, item);
      }

      return;
    }

    let elm = aItem;

    while (elm.nodeName != "richlistitem" ||
           elm.getAttribute("type") != "download") {
      elm = elm.parentNode;
    }

    let downloadItem = this._getDownloadItemForElement(elm);
    downloadItem.doCommand(aCmd);
  },

  onDragStart: function(aEvent) {
    let downloadItem = this._getSelectedDownloadItem();
    if (!downloadItem) {
      return;
    }

    let dl = this.downloadView.selectedItem;
    let f = downloadItem.localFile;
    if (!f.exists()) {
      return;
    }

    let dt = aEvent.dataTransfer;
    dt.mozSetDataAt("application/x-moz-file", f, 0);
    let url = Services.io.newFileURI(f).spec;
    dt.setData("text/uri-list", url);
    dt.setData("text/plain", url);
    dt.effectAllowed = "copyMove";
    dt.addElement(dl);
  },

  onDragOver: function(aEvent) {
    let types = aEvent.dataTransfer.types;
    if (types.contains("text/uri-list") ||
        types.contains("text/x-moz-url") ||
        types.contains("text/plain")) {
      aEvent.preventDefault();
    }

    aEvent.stopPropagation();
  },

  onDrop: function(aEvent) {
    let dt = aEvent.dataTransfer;
    
    
    if (dt.mozGetDataAt("application/x-moz-file", 0)) {
      return;
    }

    let name;
    let url = dt.getData("URL");

    if (!url) {
      url = dt.getData("text/x-moz-url") || dt.getData("text/plain");
      [url, name] = url.split("\n");
    }

    if (url) {
      let sourceDoc = dt.mozSourceNode ? dt.mozSourceNode.ownerDocument : document;
      saveURL(url, name ? name : url, null, true, true, null, sourceDoc);
    }
  },

  pasteHandler: function() {
    let trans = Cc["@mozilla.org/widget/transferable;1"].
                createInstance(Ci.nsITransferable);
    trans.init(null);
    let flavors = ["text/x-moz-url", "text/unicode"];
    flavors.forEach(trans.addDataFlavor);

    Services.clipboard.getData(trans, Services.clipboard.kGlobalClipboard);

    
    try {
      let data = {};
      trans.getAnyTransferData({}, data, {});
      let [url, name] = data.value.QueryInterface(Ci.nsISupportsString).data.split("\n");

      if (!url) {
        return;
      }

      let uri = Services.io.newURI(url, null, null);

      saveURL(uri.spec, name || uri.spec, null, true, true, null, document);
    } catch (ex) {}
  },

  selectAll: function() {
    this.downloadView.selectAll();
  },

  onUpdateProgress: function() {
    let numActive = 0;
    let totalSize = 0;
    let totalTransferred = 0;

    for (let downloadItem of this.idToDownloadItemMap.values()) {
      if (!downloadItem.inProgress) {
        continue;
      }

      numActive++;

      
      if (downloadItem.maxBytes > 0 &&
          downloadItem.state != nsIDM.DOWNLOAD_CANCELED &&
          downloadItem.state != nsIDM.DOWNLOAD_FAILED) {
        totalSize += downloadItem.maxBytes;
        totalTransferred += downloadItem.currBytes;
      }
    }

    
    if (numActive == 0) {
      document.title = document.documentElement.getAttribute("statictitle");
      return;
    }

    
    let mean = totalTransferred;

    
    let title = gStr.downloadsTitlePercent;
    if (totalSize == 0) {
      title = gStr.downloadsTitleFiles;
    } else {
      mean = Math.floor((totalTransferred / totalSize) * 100);
    }

    
    title = PluralForm.get(numActive, title);
    title = title.replace("#1", numActive);
    title = title.replace("#2", mean);

    
    document.title = title;
  },

  onDownloadAdded: function(aDownload) {
    let newID = this._autoIncrementID++;

    let downloadItem = new DownloadItem(newID, aDownload);
    this.downloadItemsMap.set(aDownload, downloadItem);
    this.idToDownloadItemMap.set(newID, downloadItem);

    if (downloadItem.inProgress ||
        downloadItem.matchesSearch(this.searchTerms, this.searchAttributes)) {
      this.downloadView.appendChild(downloadItem.element);

      
      
      downloadItem.updateView();

      
      this.updateClearListButton();
    }
  },

  onDownloadChanged: function(aDownload) {
    let downloadItem = this.downloadItemsMap.get(aDownload);
    if (!downloadItem) {
      Cu.reportError("Download doesn't exist.");
      return;
    }

    downloadItem.updateView();

    this.onUpdateProgress();

    
    this.updateClearListButton();

    if (downloadItem.done) {
      
      if (downloadItem.matchesSearch(this.searchTerms, this.searchAttributes)) {
        
        let next = this.element.nextSibling;
        while (next && next.inProgress) {
          next = next.nextSibling;
        }

        
        this.downloadView.insertBefore(this.element, next);
      } else {
        this.removeFromView(downloadItem);
      }

      return true;
    }

    return false;
  },

  onDownloadRemoved: function(aDownload) {
    let downloadItem = this.downloadItemsMap.get(aDownload);
    if (!downloadItem) {
      Cu.reportError("Download doesn't exist.");
      return;
    }

    this.downloadItemsMap.delete(aDownload);
    this.idToDownloadItemMap.delete(downloadItem.id);

    this.removeFromView(downloadItem);
  },

  removeFromView: function(aDownloadItem) {
    
    if (!aDownloadItem.element) {
      return;
    }

    let index = this.downloadView.selectedIndex;
    this.downloadView.removeChild(aDownloadItem.element);
    this.downloadView.selectedIndex = Math.min(index, this.downloadView.itemCount - 1);

    
    this.updateClearListButton();
  },
  _getDownloadItemForElement(element) {
    return this.idToDownloadItemMap.get(element.getAttribute("id"));
  },
  _getSelectedDownloadItem() {
    let dl = this.downloadView.selectedItem;
    return dl ? this._getDownloadItemForElement(dl) : null;
  },
};

function Startup() {
  
  let sb = document.getElementById("downloadStrings");
  let strings = ["paused", "cannotPause", "doneStatus", "doneSize",
                 "doneSizeUnknown", "stateFailed", "stateCanceled",
                 "stateBlocked", "stateBlockedPolicy", "stateDirty",
                 "downloadsTitleFiles", "downloadsTitlePercent",];

  for (let name of strings) {
    gStr[name] = sb.getString(name);
  }

  gDownloadList.init();

  let DownloadTaskbarProgress =
    Cu.import("resource://gre/modules/DownloadTaskbarProgress.jsm", {}).DownloadTaskbarProgress;
  DownloadTaskbarProgress.onDownloadWindowLoad(window);
}

function Shutdown() {
  Downloads.getList(Downloads.ALL)
           .then(list => list.removeView(gDownloadList))
           .catch(Cu.reportError);
}
