



let Ci = Components.interfaces, Cc = Components.classes, Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DownloadUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let gStrings = Services.strings.createBundle("chrome://browser/locale/aboutDownloads.properties");

let downloadTemplate =
"<li downloadID='{id}' role='button' state='{state}'>" +
  "<img class='icon' src='{icon}'/>" +
  "<div class='details'>" +
     "<div class='row'>" +
       
       "<xul:label class='title' crop='center' value='{target}'/>" +
       "<div class='date'>{date}</div>" +
     "</div>" +
     "<div class='size'>{size}</div>" +
     "<div class='domain'>{domain}</div>" +
     "<div class='displayState'>{displayState}</div>" +
  "</div>" +
"</li>";

XPCOMUtils.defineLazyGetter(window, "gChromeWin", function ()
  window.QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIWebNavigation)
    .QueryInterface(Ci.nsIDocShellTreeItem)
    .rootTreeItem
    .QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIDOMWindow)
    .QueryInterface(Ci.nsIDOMChromeWindow));

let Downloads = {
  init: function dl_init() {
    this._list = document.getElementById("downloads-list");
    this._list.addEventListener("click", function (event) {
      let target = event.target;
      while (target && target.nodeName != "li") {
        target = target.parentNode;
      }

      Downloads.openDownload(target);
    }, false);

    Services.obs.addObserver(this, "dl-start", false);
    Services.obs.addObserver(this, "dl-failed", false);
    Services.obs.addObserver(this, "dl-scanning", false);
    Services.obs.addObserver(this, "dl-done", false);
    Services.obs.addObserver(this, "dl-blocked", false);
    Services.obs.addObserver(this, "dl-dirty", false);
    Services.obs.addObserver(this, "dl-cancel", false);

    this.getDownloads();

    let contextmenus = gChromeWin.NativeWindow.contextmenus;

    
    Downloads.openMenuItem = contextmenus.add(gStrings.GetStringFromName("downloadAction.open"),
                                              contextmenus.SelectorContext("li[state='" + this._dlmgr.DOWNLOAD_FINISHED + "']"),
      function (aTarget) {
        Downloads.openDownload(aTarget);
      }
    );
    
    
    Downloads.retryMenuItem = contextmenus.add(gStrings.GetStringFromName("downloadAction.retry"),
                                               contextmenus.SelectorContext("li[state='" + this._dlmgr.DOWNLOAD_FAILED + "']," +
                                                                            "li[state='" + this._dlmgr.DOWNLOAD_CANCELED + "']"),
      function (aTarget) {
        Downloads.retryDownload(aTarget);
      }
    );
    
    
    Downloads.removeMenuItem = contextmenus.add(gStrings.GetStringFromName("downloadAction.remove"),
                                                contextmenus.SelectorContext("li[state='" + this._dlmgr.DOWNLOAD_CANCELED + "']," +
                                                                             "li[state='" + this._dlmgr.DOWNLOAD_FINISHED + "']," +
                                                                             "li[state='" + this._dlmgr.DOWNLOAD_FAILED + "']"),
      function (aTarget) {
        Downloads.removeDownload(aTarget);
      }
    );

    
    Downloads.pauseMenuItem = contextmenus.add(gStrings.GetStringFromName("downloadAction.pause"),
                                               contextmenus.SelectorContext("li[state='" + this._dlmgr.DOWNLOAD_DOWNLOADING + "']"),
      function (aTarget) {
        Downloads.pauseDownload(aTarget);
      }
    );
    
    
    Downloads.resumeMenuItem = contextmenus.add(gStrings.GetStringFromName("downloadAction.resume"),
                                                contextmenus.SelectorContext("li[state='" + this._dlmgr.DOWNLOAD_PAUSED + "']"),
      function (aTarget) {
        Downloads.resumeDownload(aTarget);
      }
    );
    
    
    Downloads.cancelMenuItem = contextmenus.add(gStrings.GetStringFromName("downloadAction.cancel"),
                                                contextmenus.SelectorContext("li[state='" + this._dlmgr.DOWNLOAD_DOWNLOADING + "']," +
                                                                             "li[state='" + this._dlmgr.DOWNLOAD_NOTSTARTED + "']," +
                                                                             "li[state='" + this._dlmgr.DOWNLOAD_QUEUED + "']," +
                                                                             "li[state='" + this._dlmgr.DOWNLOAD_PAUSED + "']"),
      function (aTarget) {
        Downloads.cancelDownload(aTarget);
      }
    );
  },

  uninit: function dl_uninit() {
    let contextmenus = gChromeWin.NativeWindow.contextmenus;
    contextmenus.remove(this.openMenuItem);
    contextmenus.remove(this.removeMenuItem);
    contextmenus.remove(this.pauseMenuItem);
    contextmenus.remove(this.resumeMenuItem);
    contextmenus.remove(this.retryMenuItem);
    contextmenus.remove(this.cancelMenuItem);

    Services.obs.removeObserver(this, "dl-start");
    Services.obs.removeObserver(this, "dl-failed");
    Services.obs.removeObserver(this, "dl-scanning");
    Services.obs.removeObserver(this, "dl-done");
    Services.obs.removeObserver(this, "dl-blocked");
    Services.obs.removeObserver(this, "dl-dirty");
    Services.obs.removeObserver(this, "dl-cancel");
  },

  observe: function dl_observe(aSubject, aTopic, aData) {
    let download = aSubject.QueryInterface(Ci.nsIDownload);
    switch (aTopic) {
      case "dl-blocked":
      case "dl-dirty":
      case "dl-cancel":
      case "dl-done":
      case "dl-failed":
        
        this._moveDownloadAfterActive(this._getElementForDownload(download.id));
      
      
      case "dl-start":
      case "dl-scanning":
        let item = this._getElementForDownload(download.id);
        if (item)
          this._updateDownloadRow(item);
        else
          this._insertDownloadRow(download);
        break;
    }
  },

  _moveDownloadAfterActive: function dl_moveDownloadAfterActive(aItem) {
    
    try {
      
      let next = aItem.nextSibling;
      while (next && this._inProgress(next.getAttribute("state")))
        next = next.nextSibling;
      
      this._list.insertBefore(aItem, next);
    } catch (ex) {
      console.log("ERROR: _moveDownloadAfterActive() : " + ex);
    }
  },
  
  _inProgress: function dl_inProgress(aState) {
    return [
      this._dlmgr.DOWNLOAD_NOTSTARTED,
      this._dlmgr.DOWNLOAD_QUEUED,
      this._dlmgr.DOWNLOAD_DOWNLOADING,
      this._dlmgr.DOWNLOAD_PAUSED,
      this._dlmgr.DOWNLOAD_SCANNING,
    ].indexOf(parseInt(aState)) != -1;
  },

  _insertDownloadRow: function dl_insertDownloadRow(aDownload) {
    let updatedState = this._getState(aDownload.state);
    let item = this._createItem(downloadTemplate, {
      id: aDownload.id,
      target: aDownload.displayName,
      icon: "moz-icon://" + aDownload.displayName + "?size=64",
      date: DownloadUtils.getReadableDates(new Date())[0],
      domain: DownloadUtils.getURIHost(aDownload.source.spec)[0],
      size: this._getDownloadSize(aDownload.size),
      displayState: this._getStateString(updatedState),
      state: updatedState
    });
    this._list.insertAdjacentHTML("afterbegin", item);
  },

  _getDownloadSize: function dl_getDownloadSize(aSize) {
    let displaySize = DownloadUtils.convertByteUnits(aSize);
    if (displaySize[0] > 0) 
      return displaySize.join("");
    else
      return gStrings.GetStringFromName("downloadState.unknownSize");
  },
  
  
  _getState: function dl_getState(aState) {
    let str;
    switch (aState) {
      
      case this._dlmgr.DOWNLOAD_DOWNLOADING:
      case this._dlmgr.DOWNLOAD_SCANNING:
        str = this._dlmgr.DOWNLOAD_DOWNLOADING;
        break;
        
      
      case this._dlmgr.DOWNLOAD_FAILED:
      case this._dlmgr.DOWNLOAD_DIRTY:
      case this._dlmgr.DOWNLOAD_BLOCKED_POLICY:
      case this._dlmgr.DOWNLOAD_BLOCKED_PARENTAL:
        str = this._dlmgr.DOWNLOAD_FAILED;
        break;
        
      


         
      default:
        str = aState;
    }
    return str;
  },
  
  
  _getStateString: function dl_getStateString(aState) {
    let str;
    switch (aState) {
      case this._dlmgr.DOWNLOAD_DOWNLOADING:
        str = "downloadState.downloading";
        break;
      case this._dlmgr.DOWNLOAD_CANCELED:
        str = "downloadState.canceled";
        break;
      case this._dlmgr.DOWNLOAD_FAILED:
        str = "downloadState.failed";
        break;
      case this._dlmgr.DOWNLOAD_PAUSED:
        str = "downloadState.paused";
        break;
        
      
      case this._dlmgr.DOWNLOAD_QUEUED:
      case this._dlmgr.DOWNLOAD_NOTSTARTED:
        str = "downloadState.starting";
        break;
        
      default:
        return "";
    }
    return gStrings.GetStringFromName(str);
  },

  _updateItem: function dl_updateItem(aItem, aValues) {
    for (let i in aValues) {
      aItem.querySelector("." + i).textContent = aValues[i];
    }
  },

  _initStatement: function dv__initStatement() {
    if (this._stmt)
      this._stmt.finalize();

    this._stmt = this._dlmgr.DBConnection.createStatement(
      "SELECT id, name, source, state, startTime, endTime, referrer, " +
             "currBytes, maxBytes, state IN (?1, ?2, ?3, ?4, ?5) isActive " +
      "FROM moz_downloads " +
      "ORDER BY isActive DESC, endTime DESC, startTime DESC");
  },

  _createItem: function _createItem(aTemplate, aValues) {
    function htmlEscape(s) {
      s = s.replace(/&/g, "&amp;");
      s = s.replace(/>/g, "&gt;");
      s = s.replace(/</g, "&lt;");
      s = s.replace(/"/g, "&quot;");
      s = s.replace(/'/g, "&apos;");
      return s;
    }

    let t = aTemplate;
    for (let key in aValues) {
      if (aValues.hasOwnProperty(key)) {
        let regEx = new RegExp("{" + key + "}", "g");
        let value = htmlEscape(aValues[key].toString());
        t = t.replace(regEx, value);
      }
    }
    return t;
  },

  _stepDownloads: function dv__stepDownloads(aNumItems) {
    try {
      if (!this._stmt.executeStep()) {
        this._stmt.finalize();
        this._stmt = null;
        return;
      }
  
      let updatedState = this._getState(this._stmt.row.state);
      
      let attrs = {
        id: this._stmt.row.id,
        target: this._stmt.row.name,
        icon: "moz-icon://" + this._stmt.row.name + "?size=64",
        date: DownloadUtils.getReadableDates(new Date(this._stmt.row.endTime / 1000))[0],
        domain: DownloadUtils.getURIHost(this._stmt.row.source)[0],
        size: this._getDownloadSize(this._stmt.row.maxBytes),
        displayState: this._getStateString(updatedState),
        state: updatedState
      };

      let item = this._createItem(downloadTemplate, attrs);
      this._list.insertAdjacentHTML("beforeend", item);
    } catch (e) {
      
      console.log("Error: " + e);
      this._stmt.reset();
      return;
    }

    
    
    if (aNumItems > 1) {
      this._stepDownloads(aNumItems - 1);
    } else {
      
      let delay = Math.min(this._list.itemCount * 10, 300);
      let self = this;
      this._timeoutID = setTimeout(function () { self._stepDownloads(5); }, delay);
    }
  },

  getDownloads: function dl_getDownloads() {
    this._dlmgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);

    this._initStatement();

    clearTimeout(this._timeoutID);

    this._stmt.reset();
    this._stmt.bindInt32Parameter(0, Ci.nsIDownloadManager.DOWNLOAD_NOTSTARTED);
    this._stmt.bindInt32Parameter(1, Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING);
    this._stmt.bindInt32Parameter(2, Ci.nsIDownloadManager.DOWNLOAD_PAUSED);
    this._stmt.bindInt32Parameter(3, Ci.nsIDownloadManager.DOWNLOAD_QUEUED);
    this._stmt.bindInt32Parameter(4, Ci.nsIDownloadManager.DOWNLOAD_SCANNING);

    
    let self = this;
    this._timeoutID = setTimeout(function () {
      self._stepDownloads(1);
    }, 0);
  },

  _getElementForDownload: function dl_getElementForDownload(aKey) {
    return this._list.querySelector("li[downloadID='" + aKey + "']");
  },

  _getDownloadForElement: function dl_getDownloadForElement(aElement) {
    let id = parseInt(aElement.getAttribute("downloadID"));
    return this._dlmgr.getDownload(id);
  },

  _removeItem: function dl_removeItem(aItem) {
    
    if (!aItem)
      return;
  
    let index = this._list.selectedIndex;
    this._list.removeChild(aItem);
    this._list.selectedIndex = Math.min(index, this._list.itemCount - 1);
  },
  
  openDownload: function dl_openDownload(aItem) {
    let f = null;
    try {
      let download = this._getDownloadForElement(aItem);
      f = download.targetFile;
    } catch(ex) { }

    try {
      if (f) f.launch();
    } catch (ex) { }
  },

  removeDownload: function dl_removeDownload(aItem) {
    let f = null;
    try {
      let download = this._getDownloadForElement(aItem);
      f = download.targetFile;
    } catch(ex) {
      
      
      f = { leafName: "" };
    }

    this._dlmgr.removeDownload(aItem.getAttribute("downloadID"));

    this._list.removeChild(aItem);

    try {
      if (f) f.remove(false);
    } catch(ex) { }
  },

  pauseDownload: function dl_pauseDownload(aItem) {
    try {
      let download = this._getDownloadForElement(aItem);
      this._dlmgr.pauseDownload(aItem.getAttribute("downloadID"));
      this._updateDownloadRow(aItem);
    } catch (ex) {
      console.log("Error: pauseDownload() " + ex);  
    }

  },

  resumeDownload: function dl_resumeDownload(aItem) {
    try {
      let download = this._getDownloadForElement(aItem);
      this._dlmgr.resumeDownload(aItem.getAttribute("downloadID"));
      this._updateDownloadRow(aItem);
    } catch (ex) {
      console.log("Error: resumeDownload() " + ex);  
    }
  },

  retryDownload: function dl_retryDownload(aItem) {
    try {
      let download = this._getDownloadForElement(aItem);
      this._removeItem(aItem);
      this._dlmgr.retryDownload(aItem.getAttribute("downloadID"));
    } catch (ex) {
      console.log("Error: retryDownload() " + ex);  
    }
  },

  cancelDownload: function dl_cancelDownload(aItem) {
    try {
      this._dlmgr.cancelDownload(aItem.getAttribute("downloadID"));
      let download = this._getDownloadForElement(aItem);
      let f = download.targetFile;

      if (f.exists())
        f.remove(false);
      
      this._updateDownloadRow(aItem);
    } catch (ex) {
      console.log("Error: cancelDownload() " + ex);  
    }
  },
  
  _updateDownloadRow: function dl_updateDownloadRow(aItem){
    try {
      let download = this._getDownloadForElement(aItem);
      let updatedState = this._getState(download.state);
      aItem.setAttribute("state", updatedState);
      this._updateItem(aItem, {
        size: this._getDownloadSize(download.size),
        displayState: this._getStateString(updatedState),
        date: DownloadUtils.getReadableDates(new Date())[0]
      });
    } catch (ex){
       console.log("ERROR: _updateDownloadRow(): " + ex);
    }
  }
}
