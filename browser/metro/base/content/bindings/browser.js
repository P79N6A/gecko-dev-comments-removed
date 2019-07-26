




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

let WebProgressListener = {
  _lastLocation: null,
  _firstPaint: false,

  init: function() {
    let flags = Ci.nsIWebProgress.NOTIFY_LOCATION |
                Ci.nsIWebProgress.NOTIFY_SECURITY |
                Ci.nsIWebProgress.NOTIFY_STATE_WINDOW |
                Ci.nsIWebProgress.NOTIFY_STATE_NETWORK |
                Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT;

    let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebProgress);
    webProgress.addProgressListener(this, flags);
  },

  onStateChange: function onStateChange(aWebProgress, aRequest, aStateFlags, aStatus) {
    if (content != aWebProgress.DOMWindow)
      return;

    sendAsyncMessage("Content:StateChange", {
      contentWindowId: this.contentWindowId,
      stateFlags: aStateFlags,
      status: aStatus
    });
  },

  onLocationChange: function onLocationChange(aWebProgress, aRequest, aLocationURI, aFlags) {
    if (content != aWebProgress.DOMWindow)
      return;

    let spec = aLocationURI ? aLocationURI.spec : "";
    let location = spec.split("#")[0];

    let charset = content.document.characterSet;

    sendAsyncMessage("Content:LocationChange", {
      contentWindowId: this.contentWindowId,
      documentURI:     aWebProgress.DOMWindow.document.documentURIObject.spec,
      location:        spec,
      canGoBack:       docShell.canGoBack,
      canGoForward:    docShell.canGoForward,
      charset:         charset.toString()
    });

    this._firstPaint = false;
    let self = this;

    
    this.hashChanged = (location == this._lastLocation);
    this._lastLocation = location;

    
    addEventListener("MozAfterPaint", function(aEvent) {
      removeEventListener("MozAfterPaint", arguments.callee, true);

      self._firstPaint = true;
      let scrollOffset = ContentScroll.getScrollOffset(content);
      sendAsyncMessage("Browser:FirstPaint", scrollOffset);
    }, true);
  },

  onStatusChange: function onStatusChange(aWebProgress, aRequest, aStatus, aMessage) {
  },

  onSecurityChange: function onSecurityChange(aWebProgress, aRequest, aState) {
    if (content != aWebProgress.DOMWindow)
      return;

    let serialization = SecurityUI.getSSLStatusAsString();

    sendAsyncMessage("Content:SecurityChange", {
      contentWindowId: this.contentWindowId,
      SSLStatusAsString: serialization,
      state: aState
    });
  },

  get contentWindowId() {
    return content.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIDOMWindowUtils)
                  .currentInnerWindowID;
  },

  QueryInterface: function QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIWebProgressListener) ||
        aIID.equals(Ci.nsISupportsWeakReference) ||
        aIID.equals(Ci.nsISupports)) {
        return this;
    }

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

WebProgressListener.init();


let SecurityUI = {
  getSSLStatusAsString: function() {
    let status = docShell.securityUI.QueryInterface(Ci.nsISSLStatusProvider).SSLStatus;

    if (status) {
      let serhelper = Cc["@mozilla.org/network/serialization-helper;1"]
                      .getService(Ci.nsISerializationHelper);

      status.QueryInterface(Ci.nsISerializable);
      return serhelper.serializeToString(status);
    }

    return null;
  }
};

let WebNavigation =  {
  _webNavigation: docShell.QueryInterface(Ci.nsIWebNavigation),
  _timer: null,

  init: function() {
    addMessageListener("WebNavigation:GoBack", this);
    addMessageListener("WebNavigation:GoForward", this);
    addMessageListener("WebNavigation:GotoIndex", this);
    addMessageListener("WebNavigation:LoadURI", this);
    addMessageListener("WebNavigation:Reload", this);
    addMessageListener("WebNavigation:Stop", this);
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case "WebNavigation:GoBack":
        this.goBack();
        break;
      case "WebNavigation:GoForward":
        this.goForward();
        break;
      case "WebNavigation:GotoIndex":
        this.gotoIndex(message);
        break;
      case "WebNavigation:LoadURI":
        this.loadURI(message);
        break;
      case "WebNavigation:Reload":
        this.reload(message);
        break;
      case "WebNavigation:Stop":
        this.stop(message);
        break;
    }
  },

  goBack: function() {
    if (this._webNavigation.canGoBack)
      this._webNavigation.goBack();
  },

  goForward: function() {
    if (this._webNavigation.canGoForward)
      this._webNavigation.goForward();
  },

  gotoIndex: function(message) {
    this._webNavigation.gotoIndex(message.index);
  },

  loadURI: function(message) {
    let flags = message.json.flags || this._webNavigation.LOAD_FLAGS_NONE;
    this._webNavigation.loadURI(message.json.uri, flags, null, null, null);

    let tabData = message.json;
    if (tabData.entries) {
      
      
      this._webNavigation.stop(this._webNavigation.STOP_ALL);
      this._restoreHistory(tabData, 0);
    }
  },

  reload: function(message) {
    let flags = message.json.flags || this._webNavigation.LOAD_FLAGS_NONE;
    this._webNavigation.reload(flags);
  },

  stop: function(message) {
    let flags = message.json.flags || this._webNavigation.STOP_ALL;
    this._webNavigation.stop(flags);
  },

  _restoreHistory: function _restoreHistory(aTabData, aCount) {
    
    
    try {
      if (!this._webNavigation.sessionHistory)
        throw new Error();
    } catch (ex) {
      if (aCount < 10) {
        let self = this;
        this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        this._timer.initWithCallback(function(aTimer) {
          self._timer = null;
          self._restoreHistory(aTabData, aCount + 1);
        }, 100, Ci.nsITimer.TYPE_ONE_SHOT);
        return;
      }
    }

    let history = this._webNavigation.sessionHistory;
    if (history.count > 0)
      history.PurgeHistory(history.count);
    history.QueryInterface(Ci.nsISHistoryInternal);

    
    
    let idMap = { used: {} };
    let docIdentMap = {};

    for (let i = 0; i < aTabData.entries.length; i++) {
      if (!aTabData.entries[i].url)
        continue;
      history.addEntry(this._deserializeHistoryEntry(aTabData.entries[i], idMap, docIdentMap), true);
    }

    
    
    let activeIndex = (aTabData.index || aTabData.entries.length) - 1;
    history.getEntryAtIndex(activeIndex, true);
    history.QueryInterface(Ci.nsISHistory).reloadCurrentEntry();
  },

  _deserializeHistoryEntry: function _deserializeHistoryEntry(aEntry, aIdMap, aDocIdentMap) {
    let shEntry = Cc["@mozilla.org/browser/session-history-entry;1"].createInstance(Ci.nsISHEntry);

    shEntry.setURI(Services.io.newURI(aEntry.url, null, null));
    shEntry.setTitle(aEntry.title || aEntry.url);
    if (aEntry.subframe)
      shEntry.setIsSubFrame(aEntry.subframe || false);
    shEntry.loadType = Ci.nsIDocShellLoadInfo.loadHistory;
    if (aEntry.contentType)
      shEntry.contentType = aEntry.contentType;
    if (aEntry.referrer)
      shEntry.referrerURI = Services.io.newURI(aEntry.referrer, null, null);

    if (aEntry.cacheKey) {
      let cacheKey = Cc["@mozilla.org/supports-PRUint32;1"].createInstance(Ci.nsISupportsPRUint32);
      cacheKey.data = aEntry.cacheKey;
      shEntry.cacheKey = cacheKey;
    }

    if (aEntry.ID) {
      
      
      let id = aIdMap[aEntry.ID] || 0;
      if (!id) {
        for (id = Date.now(); id in aIdMap.used; id++);
        aIdMap[aEntry.ID] = id;
        aIdMap.used[id] = true;
      }
      shEntry.ID = id;
    }

    if (aEntry.docshellID)
      shEntry.docshellID = aEntry.docshellID;

    if (aEntry.structuredCloneState && aEntry.structuredCloneVersion) {
      shEntry.stateData =
        Cc["@mozilla.org/docshell/structured-clone-container;1"].
        createInstance(Ci.nsIStructuredCloneContainer);

      shEntry.stateData.initFromBase64(aEntry.structuredCloneState, aEntry.structuredCloneVersion);
    }

    if (aEntry.scroll) {
      let scrollPos = aEntry.scroll.split(",");
      scrollPos = [parseInt(scrollPos[0]) || 0, parseInt(scrollPos[1]) || 0];
      shEntry.setScrollPosition(scrollPos[0], scrollPos[1]);
    }

    let childDocIdents = {};
    if (aEntry.docIdentifier) {
      
      
      
      
      let matchingEntry = aDocIdentMap[aEntry.docIdentifier];
      if (!matchingEntry) {
        matchingEntry = {shEntry: shEntry, childDocIdents: childDocIdents};
        aDocIdentMap[aEntry.docIdentifier] = matchingEntry;
      } else {
        shEntry.adoptBFCacheEntry(matchingEntry);
        childDocIdents = matchingEntry.childDocIdents;
      }
    }

    if (aEntry.owner_b64) {
      let ownerInput = Cc["@mozilla.org/io/string-input-stream;1"].createInstance(Ci.nsIStringInputStream);
      let binaryData = atob(aEntry.owner_b64);
      ownerInput.setData(binaryData, binaryData.length);
      let binaryStream = Cc["@mozilla.org/binaryinputstream;1"].createInstance(Ci.nsIObjectInputStream);
      binaryStream.setInputStream(ownerInput);
      try { 
        shEntry.owner = binaryStream.readObject(true);
      } catch (ex) { dump(ex); }
    }

    if (aEntry.children && shEntry instanceof Ci.nsISHContainer) {
      for (let i = 0; i < aEntry.children.length; i++) {
        if (!aEntry.children[i].url)
          continue;

        
        
        
        
        
        
        
        
        
        
        
        
        
        

        shEntry.AddChild(this._deserializeHistoryEntry(aEntry.children[i], aIdMap, childDocIdents), i);
      }
    }
    
    return shEntry;
  },

  sendHistory: function sendHistory() {
    
    let entries = [];
    let history = docShell.QueryInterface(Ci.nsIWebNavigation).sessionHistory;
    for (let i = 0; i < history.count; i++) {
      let entry = this._serializeHistoryEntry(history.getEntryAtIndex(i, false));
      entries.push(entry);
    }
    let index = history.index + 1;
    sendAsyncMessage("Content:SessionHistory", { entries: entries, index: index });
  },

  _serializeHistoryEntry: function _serializeHistoryEntry(aEntry) {
    let entry = { url: aEntry.URI.spec };

    if (Util.isURLEmpty(entry.url)) {
      entry.title = Util.getEmptyURLTabTitle();
    } else {
      entry.title = aEntry.title;
    }

    if (!(aEntry instanceof Ci.nsISHEntry))
      return entry;

    let cacheKey = aEntry.cacheKey;
    if (cacheKey && cacheKey instanceof Ci.nsISupportsPRUint32 && cacheKey.data != 0)
      entry.cacheKey = cacheKey.data;

    entry.ID = aEntry.ID;
    entry.docshellID = aEntry.docshellID;

    if (aEntry.referrerURI)
      entry.referrer = aEntry.referrerURI.spec;

    if (aEntry.contentType)
      entry.contentType = aEntry.contentType;

    let x = {}, y = {};
    aEntry.getScrollPosition(x, y);
    if (x.value != 0 || y.value != 0)
      entry.scroll = x.value + "," + y.value;

    if (aEntry.owner) {
      try {
        let binaryStream = Cc["@mozilla.org/binaryoutputstream;1"].createInstance(Ci.nsIObjectOutputStream);
        let pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
        pipe.init(false, false, 0, 0xffffffff, null);
        binaryStream.setOutputStream(pipe.outputStream);
        binaryStream.writeCompoundObject(aEntry.owner, Ci.nsISupports, true);
        binaryStream.close();

        
        let scriptableStream = Cc["@mozilla.org/binaryinputstream;1"].createInstance(Ci.nsIBinaryInputStream);
        scriptableStream.setInputStream(pipe.inputStream);
        let ownerBytes = scriptableStream.readByteArray(scriptableStream.available());
        
        
        
        entry.owner_b64 = btoa(String.fromCharCode.apply(null, ownerBytes));
      } catch (e) { dump(e); }
    }

    entry.docIdentifier = aEntry.BFCacheEntry.ID;

    if (aEntry.stateData != null) {
      entry.structuredCloneState = aEntry.stateData.getDataAsBase64();
      entry.structuredCloneVersion = aEntry.stateData.formatVersion;
    }

    if (!(aEntry instanceof Ci.nsISHContainer))
      return entry;

    if (aEntry.childCount > 0) {
      entry.children = [];
      for (let i = 0; i < aEntry.childCount; i++) {
        let child = aEntry.GetChildAt(i);
        if (child)
          entry.children.push(this._serializeHistoryEntry(child));
        else 
          entry.children.push({ url: "about:blank" });

        
        if (/^wyciwyg:\/\//.test(entry.children[i].url)) {
          delete entry.children;
          break;
        }
      }
    }

    return entry;
  }
};

WebNavigation.init();


let DOMEvents =  {
  init: function() {
    addEventListener("DOMContentLoaded", this, false);
    addEventListener("DOMTitleChanged", this, false);
    addEventListener("DOMLinkAdded", this, false);
    addEventListener("DOMWillOpenModalDialog", this, false);
    addEventListener("DOMModalDialogClosed", this, true);
    addEventListener("DOMWindowClose", this, false);
    addEventListener("DOMPopupBlocked", this, false);
    addEventListener("pageshow", this, false);
    addEventListener("pagehide", this, false);
  },

  handleEvent: function(aEvent) {
    let document = content.document;
    switch (aEvent.type) {
      case "DOMContentLoaded":
        if (document.documentURIObject.spec == "about:blank")
          return;

        sendAsyncMessage("DOMContentLoaded", { });

        
        WebNavigation.sendHistory();
        break;

      case "pageshow":
      case "pagehide": {
        if (aEvent.target.defaultView != content)
          break;

        let util = aEvent.target.defaultView.QueryInterface(Ci.nsIInterfaceRequestor)
                                            .getInterface(Ci.nsIDOMWindowUtils);

        let json = {
          contentWindowWidth: content.innerWidth,
          contentWindowHeight: content.innerHeight,
          windowId: util.outerWindowID,
          persisted: aEvent.persisted
        };

        
        
        
        let contentWindowID = content.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
        if (!WebProgressListener.hashChanged && contentWindowID == util.currentInnerWindowID) {
          let focusManager = Cc["@mozilla.org/focus-manager;1"].getService(Ci.nsIFocusManager);
          focusManager.clearFocus(content);
        }

        sendAsyncMessage(aEvent.type, json);
        break;
      }

      case "DOMPopupBlocked": {
        let util = aEvent.requestingWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                          .getInterface(Ci.nsIDOMWindowUtils);
        let json = {
          windowId: util.outerWindowID,
          popupWindowURI: {
            spec: aEvent.popupWindowURI.spec,
            charset: aEvent.popupWindowURI.originCharset
          },
          popupWindowFeatures: aEvent.popupWindowFeatures,
          popupWindowName: aEvent.popupWindowName
        };

        sendAsyncMessage("DOMPopupBlocked", json);
        break;
      }

      case "DOMTitleChanged":
        sendAsyncMessage("DOMTitleChanged", { title: document.title });
        break;

      case "DOMLinkAdded":
        let target = aEvent.originalTarget;
        if (!target.href || target.disabled)
          return;

        let json = {
          windowId: target.ownerDocument.defaultView.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID,
          href: target.href,
          charset: document.characterSet,
          title: target.title,
          rel: target.rel,
          type: target.type
        };
        
        
        if (target.hasAttribute("sizes"))
          json.sizes = target.getAttribute("sizes");

        sendAsyncMessage("DOMLinkAdded", json);
        break;

      case "DOMWillOpenModalDialog":
      case "DOMModalDialogClosed":
      case "DOMWindowClose":
        let retvals = sendSyncMessage(aEvent.type, { });
        for (let i in retvals) {
          if (retvals[i].preventDefault) {
            aEvent.preventDefault();
            break;
          }
        }
        break;
    }
  }
};

DOMEvents.init();

let ContentScroll =  {
  
  _scrollOffset: { x: 0, y: 0 },

  init: function() {
    addMessageListener("Content:SetCacheViewport", this);
    addMessageListener("Content:SetWindowSize", this);

    if (Services.prefs.getBoolPref("layers.async-pan-zoom.enabled")) {
      addEventListener("scroll", this, false);
    }
    addEventListener("pagehide", this, false);
    addEventListener("MozScrolledAreaChanged", this, false);
  },

  getScrollOffset: function(aWindow) {
    let cwu = aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    let scrollX = {}, scrollY = {};
    cwu.getScrollXY(false, scrollX, scrollY);
    return { x: scrollX.value, y: scrollY.value };
  },

  getScrollOffsetForElement: function(aElement) {
    if (aElement.parentNode == aElement.ownerDocument)
      return this.getScrollOffset(aElement.ownerDocument.defaultView);
    return { x: aElement.scrollLeft, y: aElement.scrollTop };
  },

  setScrollOffsetForElement: function(aElement, aLeft, aTop) {
    if (aElement.parentNode == aElement.ownerDocument) {
      aElement.ownerDocument.defaultView.scrollTo(aLeft, aTop);
    } else {
      aElement.scrollLeft = aLeft;
      aElement.scrollTop = aTop;
    }
  },

  receiveMessage: function(aMessage) {
    let json = aMessage.json;
    switch (aMessage.name) {
      case "Content:SetCacheViewport": {
        
        let rootCwu = content.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
        if (json.id == 1) {
          rootCwu.setResolution(json.scale, json.scale);
          if (!WebProgressListener._firstPaint)
            break;
        }

        let displayport = new Rect(json.x, json.y, json.w, json.h);
        if (displayport.isEmpty())
          break;

        
        let element = null;
        try {
          element = rootCwu.findElementWithViewId(json.id);
        } catch(e) {
          
          
        }

        if (!element)
          break;

        let binding = element.ownerDocument.getBindingParent(element);
        if (binding instanceof Ci.nsIDOMHTMLInputElement && binding.mozIsTextField(false))
          break;

        
        if (json.scrollX >= 0 || json.scrollY >= 0) {
          this.setScrollOffsetForElement(element, json.scrollX, json.scrollY);
          if (element == content.document.documentElement) {
            
            
            
            this._scrollOffset = this.getScrollOffset(content);
          }
        }

        
        
        let scrollOffset = this.getScrollOffsetForElement(element);
        let x = displayport.x - scrollOffset.x;
        let y = displayport.y - scrollOffset.y;

        if (json.id == 1) {
          x = Math.round(x * json.scale) / json.scale;
          y = Math.round(y * json.scale) / json.scale;
        }

        let win = element.ownerDocument.defaultView;
        let winCwu = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
        winCwu.setDisplayPortForElement(x, y, displayport.width, displayport.height, element);
        break;
      }

      case "Content:SetWindowSize": {
        let cwu = content.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
        cwu.setCSSViewport(json.width, json.height);
        break;
      }
    }
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "pagehide":
        this._scrollOffset = { x: 0, y: 0 };
        break;

      case "scroll": {
        this.sendScroll(aEvent.target);
        break;
      }

      case "MozScrolledAreaChanged": {
        let doc = aEvent.originalTarget;
        if (content != doc.defaultView) 
          return;

        sendAsyncMessage("MozScrolledAreaChanged", {
          width: aEvent.width,
          height: aEvent.height,
          left: aEvent.x + content.scrollX
        });

        
        
        addEventListener("MozAfterPaint", function afterPaint() {
          removeEventListener("MozAfterPaint", afterPaint, false);
          sendAsyncMessage("Content:UpdateDisplayPort");
        }, false);

        break;
      }
    }
  },

  sendScroll: function sendScroll(target) {
    let isRoot = false;
    if (target instanceof Ci.nsIDOMDocument) {
      var window = target.defaultView;
      var scrollOffset = this.getScrollOffset(window);
      var element = target.documentElement;

      if (target == content.document) {
        if (this._scrollOffset.x == scrollOffset.x && this._scrollOffset.y == scrollOffset.y) {
          
          
          
          return;
        }
        this._scrollOffset = scrollOffset;
        isRoot = true;
      }
    } else {
      var window = target.currentDoc.defaultView;
      var scrollOffset = this.getScrollOffsetForElement(target);
      var element = target;
    }

    let utils = window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    let presShellId = {};
    utils.getPresShellId(presShellId);
    let viewId = utils.getViewId(element);

    sendAsyncMessage("scroll", { presShellId: presShellId.value,
                                 viewId: viewId,
                                 scrollOffset: scrollOffset,
                                 isRoot: isRoot });
  }
};

ContentScroll.init();

let ContentActive =  {
  init: function() {
    addMessageListener("Content:Activate", this);
    addMessageListener("Content:Deactivate", this);
  },

  receiveMessage: function(aMessage) {
    let json = aMessage.json;
    switch (aMessage.name) {
      case "Content:Deactivate":
        docShell.isActive = false;
        let cwu = content.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
        if (json.keepviewport)
          break;
        cwu.setDisplayPortForElement(0, 0, 0, 0, content.document.documentElement);
        break;

      case "Content:Activate":
        docShell.isActive = true;
        break;
    }
  }
};

ContentActive.init();







let IndexedDB = {
  _permissionsPrompt: "indexedDB-permissions-prompt",
  _permissionsResponse: "indexedDB-permissions-response",

  _quotaPrompt: "indexedDB-quota-prompt",
  _quotaResponse: "indexedDB-quota-response",
  _quotaCancel: "indexedDB-quota-cancel",

  waitingObservers: [],

  init: function IndexedDBPromptHelper_init() {
    let os = Services.obs;
    os.addObserver(this, this._permissionsPrompt, false);
    os.addObserver(this, this._quotaPrompt, false);
    os.addObserver(this, this._quotaCancel, false);
    addMessageListener("IndexedDB:Response", this);
  },

  observe: function IndexedDBPromptHelper_observe(aSubject, aTopic, aData) {
    if (aTopic != this._permissionsPrompt && aTopic != this._quotaPrompt && aTopic != this._quotaCancel) {
      throw new Error("Unexpected topic!");
    }

    let requestor = aSubject.QueryInterface(Ci.nsIInterfaceRequestor);
    let observer = requestor.getInterface(Ci.nsIObserver);

    let contentWindow = requestor.getInterface(Ci.nsIDOMWindow);
    let contentDocument = contentWindow.document;

    if (aTopic == this._quotaCancel) {
      observer.observe(null, this._quotaResponse, Ci.nsIPermissionManager.UNKNOWN_ACTION);
      return;
    }

    
    sendAsyncMessage("IndexedDB:Prompt", {
      topic: aTopic,
      host: contentDocument.documentURIObject.asciiHost,
      location: contentDocument.location.toString(),
      data: aData,
      observerId: this.addWaitingObserver(observer)
    });
  },

  receiveMessage: function(aMessage) {
    let payload = aMessage.json;
    switch (aMessage.name) {
      case "IndexedDB:Response":
        let observer = this.getAndRemoveWaitingObserver(payload.observerId);
        observer.observe(null, payload.responseTopic, payload.permission);
    }
  },

  addWaitingObserver: function(aObserver) {
    let observerId = 0;
    while (observerId in this.waitingObservers)
      observerId++;
    this.waitingObservers[observerId] = aObserver;
    return observerId;
  },

  getAndRemoveWaitingObserver: function(aObserverId) {
    let observer = this.waitingObservers[aObserverId];
    delete this.waitingObservers[aObserverId];
    return observer;
  }
};

IndexedDB.init();

