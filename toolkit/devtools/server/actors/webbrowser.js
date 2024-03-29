





"use strict";

let { Ci, Cu } = require("chrome");
let Services = require("Services");
let promise = require("promise");
let { ActorPool, createExtraActors, appendExtraActors } = require("devtools/server/actors/common");
let { DebuggerServer } = require("devtools/server/main");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
let { dbg_assert } = DevToolsUtils;
let { TabSources } = require("./utils/TabSources");
let makeDebugger = require("./utils/make-debugger");
let { WorkerActorList } = require("devtools/server/actors/worker");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

loader.lazyRequireGetter(this, "RootActor", "devtools/server/actors/root", true);
loader.lazyRequireGetter(this, "ThreadActor", "devtools/server/actors/script", true);
loader.lazyRequireGetter(this, "unwrapDebuggerObjectGlobal", "devtools/server/actors/script", true);
loader.lazyRequireGetter(this, "BrowserAddonActor", "devtools/server/actors/addon", true);
loader.lazyImporter(this, "AddonManager", "resource://gre/modules/AddonManager.jsm");




loader.lazyRequireGetter(this, "events", "sdk/event/core");

loader.lazyRequireGetter(this, "StyleSheetActor", "devtools/server/actors/stylesheets", true);

function getWindowID(window) {
  return window.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIDOMWindowUtils)
               .currentInnerWindowID;
}

function getDocShellChromeEventHandler(docShell) {
  let handler = docShell.chromeEventHandler;
  if (!handler) {
    try {
      
      
      handler = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIDOMWindow);
    } catch(e) {}
  }
  return handler;
}
function getChildDocShells(docShell) {
  let docShellsEnum = docShell.getDocShellEnumerator(
    Ci.nsIDocShellTreeItem.typeAll,
    Ci.nsIDocShell.ENUMERATE_FORWARDS
  );

  let docShells = [];
  while (docShellsEnum.hasMoreElements()) {
    let docShell = docShellsEnum.getNext();
    docShell.QueryInterface(Ci.nsIInterfaceRequestor)
            .getInterface(Ci.nsIWebProgress);
    docShells.push(docShell);
  }
  return docShells;
}
exports.getChildDocShells = getChildDocShells;





function getInnerId(window) {
  return window.QueryInterface(Ci.nsIInterfaceRequestor).
                getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
};






function* allAppShellDOMWindows(aWindowType)
{
  let e = Services.wm.getEnumerator(aWindowType);
  while (e.hasMoreElements()) {
    yield e.getNext();
  }
}

exports.allAppShellDOMWindows = allAppShellDOMWindows;




function appShellDOMWindowType(aWindow) {
  
  return aWindow.document.documentElement.getAttribute('windowtype');
}




function sendShutdownEvent() {
  for (let win of allAppShellDOMWindows(DebuggerServer.chromeWindowType)) {
    let evt = win.document.createEvent("Event");
    evt.initEvent("Debugger:Shutdown", true, false);
    win.document.documentElement.dispatchEvent(evt);
  }
}

exports.sendShutdownEvent = sendShutdownEvent;












function createRootActor(aConnection)
{
  return new RootActor(aConnection,
                       {
                         tabList: new BrowserTabList(aConnection),
                         addonList: new BrowserAddonList(aConnection),
                         globalActorFactories: DebuggerServer.globalActorFactories,
                         onShutdown: sendShutdownEvent
                       });
}







































































function BrowserTabList(aConnection)
{
  this._connection = aConnection;

  

























  this._actorByBrowser = new Map();

  
  this._onListChanged = null;

  



  this._mustNotify = false;

  
  this._testing = false;
}

BrowserTabList.prototype.constructor = BrowserTabList;












BrowserTabList.prototype._getSelectedBrowser = function(aWindow) {
  return aWindow.gBrowser ? aWindow.gBrowser.selectedBrowser : null;
};





BrowserTabList.prototype._getBrowsers = function*() {
  
  for (let win of allAppShellDOMWindows(DebuggerServer.chromeWindowType)) {
    
    
    
    
    for (let browser of this._getChildren(win)) {
      yield browser;
    }
  }
};

BrowserTabList.prototype._getChildren = function(aWindow) {
  let children = aWindow.gBrowser ? aWindow.gBrowser.browsers : [];
  return children ? children : [];
};

BrowserTabList.prototype._isRemoteBrowser = function(browser) {
  return browser.getAttribute("remote") == "true";
};

BrowserTabList.prototype.getList = function() {
  let topXULWindow = Services.wm.getMostRecentWindow(DebuggerServer.chromeWindowType);
  let selectedBrowser = null;
  if (topXULWindow) {
    selectedBrowser = this._getSelectedBrowser(topXULWindow);
  }

  
  
  let initialMapSize = this._actorByBrowser.size;
  this._foundCount = 0;

  
  
  
  

  let actorPromises = [];

  for (let browser of this._getBrowsers()) {
    let selected = browser === selectedBrowser;
    actorPromises.push(
      this._getActorForBrowser(browser)
          .then(actor => {
            
            actor.selected = selected;
            return actor;
          })
    );
  }

  if (this._testing && initialMapSize !== this._foundCount)
    throw Error("_actorByBrowser map contained actors for dead tabs");

  this._mustNotify = true;
  this._checkListening();

  return promise.all(actorPromises);
};

BrowserTabList.prototype._getActorForBrowser = function(browser) {
  
  let actor = this._actorByBrowser.get(browser);
  if (actor) {
    this._foundCount++;
    return actor.update();
  } else if (this._isRemoteBrowser(browser)) {
    actor = new RemoteBrowserTabActor(this._connection, browser);
    this._actorByBrowser.set(browser, actor);
    this._checkListening();
    return actor.connect();
  } else {
    actor = new BrowserTabActor(this._connection, browser,
                                browser.getTabBrowser());
    this._actorByBrowser.set(browser, actor);
    this._checkListening();
    return promise.resolve(actor);
  }
};

BrowserTabList.prototype.getTab = function({ outerWindowID, tabId }) {
  if (typeof(outerWindowID) == "number") {
    
    for (let browser of this._getBrowsers()) {
      if (browser.contentWindow) {
        let windowUtils = browser.contentWindow
          .QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIDOMWindowUtils);
        if (windowUtils.outerWindowID === outerWindowID) {
          return this._getActorForBrowser(browser);
        }
      }
    }
    return promise.reject({
      error: "noTab",
      message: "Unable to find tab with outerWindowID '" + outerWindowID + "'"
    });
  } else if (typeof(tabId) == "number") {
    
    for (let browser of this._getBrowsers()) {
      if (browser.frameLoader.tabParent &&
          browser.frameLoader.tabParent.tabId === tabId) {
        return this._getActorForBrowser(browser);
      }
    }
    return promise.reject({
      error: "noTab",
      message: "Unable to find tab with tabId '" + tabId + "'"
    });
  }

  let topXULWindow = Services.wm.getMostRecentWindow(DebuggerServer.chromeWindowType);
  if (topXULWindow) {
    let selectedBrowser = this._getSelectedBrowser(topXULWindow);
    return this._getActorForBrowser(selectedBrowser);
  }
  return promise.reject({
    error: "noTab",
    message: "Unable to find any selected browser"
  });
};

Object.defineProperty(BrowserTabList.prototype, 'onListChanged', {
  enumerable: true, configurable:true,
  get: function() { return this._onListChanged; },
  set: function(v) {
    if (v !== null && typeof v !== 'function') {
      throw Error("onListChanged property may only be set to 'null' or a function");
    }
    this._onListChanged = v;
    this._checkListening();
  }
});





BrowserTabList.prototype._notifyListChanged = function() {
  if (!this._onListChanged)
    return;
  if (this._mustNotify) {
    this._onListChanged();
    this._mustNotify = false;
  }
};





BrowserTabList.prototype._handleActorClose = function(aActor, aBrowser) {
  if (this._testing) {
    if (this._actorByBrowser.get(aBrowser) !== aActor) {
      throw Error("BrowserTabActor not stored in map under given browser");
    }
    if (aActor.browser !== aBrowser) {
      throw Error("actor's browser and map key don't match");
    }
  }

  this._actorByBrowser.delete(aBrowser);
  aActor.exit();

  this._notifyListChanged();
  this._checkListening();
};







BrowserTabList.prototype._checkListening = function() {
  









  this._listenForEventsIf(this._onListChanged && this._mustNotify,
                          "_listeningForTabOpen", ["TabOpen", "TabSelect"]);

  
  this._listenForEventsIf(this._actorByBrowser.size > 0,
                          "_listeningForTabClose", ["TabClose"]);

  




  this._listenToMediatorIf((this._onListChanged && this._mustNotify) ||
                           (this._actorByBrowser.size > 0));
};












BrowserTabList.prototype._listenForEventsIf = function(aShouldListen, aGuard, aEventNames) {
  if (!aShouldListen !== !this[aGuard]) {
    let op = aShouldListen ? "addEventListener" : "removeEventListener";
    for (let win of allAppShellDOMWindows(DebuggerServer.chromeWindowType)) {
      for (let name of aEventNames) {
        win[op](name, this, false);
      }
    }
    this[aGuard] = aShouldListen;
  }
};




BrowserTabList.prototype.handleEvent = DevToolsUtils.makeInfallible(function(aEvent) {
  switch (aEvent.type) {
  case "TabOpen":
  case "TabSelect":
    
    this._notifyListChanged();
    this._checkListening();
    break;
  case "TabClose":
    let browser = aEvent.target.linkedBrowser;
    let actor = this._actorByBrowser.get(browser);
    if (actor) {
      this._handleActorClose(actor, browser);
    }
    break;
  }
}, "BrowserTabList.prototype.handleEvent");





BrowserTabList.prototype._listenToMediatorIf = function(aShouldListen) {
  if (!aShouldListen !== !this._listeningToMediator) {
    let op = aShouldListen ? "addListener" : "removeListener";
    Services.wm[op](this);
    this._listeningToMediator = aShouldListen;
  }
};










BrowserTabList.prototype.onWindowTitleChange = () => { };

BrowserTabList.prototype.onOpenWindow = DevToolsUtils.makeInfallible(function(aWindow) {
  let handleLoad = DevToolsUtils.makeInfallible(() => {
    
    aWindow.removeEventListener("load", handleLoad, false);

    if (appShellDOMWindowType(aWindow) !== DebuggerServer.chromeWindowType)
      return;

    
    if (this._listeningForTabOpen) {
      aWindow.addEventListener("TabOpen", this, false);
      aWindow.addEventListener("TabSelect", this, false);
    }
    if (this._listeningForTabClose) {
      aWindow.addEventListener("TabClose", this, false);
    }

    
    
    
    this._notifyListChanged();
  });

  





  aWindow = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindow);

  aWindow.addEventListener("load", handleLoad, false);
}, "BrowserTabList.prototype.onOpenWindow");

BrowserTabList.prototype.onCloseWindow = DevToolsUtils.makeInfallible(function(aWindow) {
  aWindow = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindow);

  if (appShellDOMWindowType(aWindow) !== DebuggerServer.chromeWindowType)
    return;

  




  Services.tm.currentThread.dispatch(DevToolsUtils.makeInfallible(() => {
    



    for (let [browser, actor] of this._actorByBrowser) {
      
      if (!browser.ownerDocument.defaultView) {
        this._handleActorClose(actor, browser);
      }
    }
  }, "BrowserTabList.prototype.onCloseWindow's delayed body"), 0);
}, "BrowserTabList.prototype.onCloseWindow");

exports.BrowserTabList = BrowserTabList;




































































































function TabActor(aConnection)
{
  this.conn = aConnection;
  this._tabActorPool = null;
  
  this._extraActors = {};
  this._exited = false;
  this._sources = null;

  
  this._styleSheetActors = new Map();

  this._shouldAddNewGlobalAsDebuggee = this._shouldAddNewGlobalAsDebuggee.bind(this);

  this.makeDebugger = makeDebugger.bind(null, {
    findDebuggees: () => this.windows,
    shouldAddNewGlobalAsDebuggee: this._shouldAddNewGlobalAsDebuggee
  });

  
  
  this.listenForNewDocShells = Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT;

  this.traits = {
    reconfigure: true,
    
    
    frames: true,
    
    
    noTabReconfigureOnClose: true
  };

  this._workerActorList = null;
  this._workerActorPool = null;
  this._onWorkerActorListChanged = this._onWorkerActorListChanged.bind(this);
}




TabActor.prototype = {
  traits: null,

  get exited() { return this._exited; },
  get attached() { return !!this._attached; },

  _tabPool: null,
  get tabActorPool() { return this._tabPool; },

  _contextPool: null,
  get contextActorPool() { return this._contextPool; },

  _pendingNavigation: null,

  
  actorPrefix: "tab",

  


  get chromeEventHandler() {
    return getDocShellChromeEventHandler(this.docShell);
  },

  


  get messageManager() {
    return this.docShell
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIContentFrameMessageManager);
  },

  


  get docShell() {
    throw "The docShell getter should be implemented by a subclass of TabActor";
  },

  



  get docShells() {
    return getChildDocShells(this.docShell);
  },

  


  get window() {
    
    if (this.docShell) {
      return this.docShell
        .QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindow);
    }
    return null;
  },

  



  get windows() {
    return this.docShells.map(docShell => {
      return docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindow);
    });
  },

  






  get originalDocShell() {
    if (!this._originalWindow) {
      return this.docShell;
    }

    return this._originalWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIWebNavigation)
                               .QueryInterface(Ci.nsIDocShell);
  },

  






  get originalWindow() {
    return this._originalWindow || this.window;
  },

  


  get webProgress() {
    return this.docShell
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebProgress);
  },

  


  get webNavigation() {
    return this.docShell
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation);
  },

  


  get contentDocument() {
    return this.webNavigation.document;
  },

  




  get title() {
    return this.contentDocument.contentTitle;
  },

  




  get url() {
    if (this.webNavigation.currentURI) {
      return this.webNavigation.currentURI.spec;
    }
    
    
    return null;
  },

  get sources() {
    if (!this._sources) {
      dbg_assert(this.threadActor, "threadActor should exist when creating sources.");
      this._sources = new TabSources(this.threadActor);
    }
    return this._sources;
  },

  




  update: function() {
    return promise.resolve(this);
  },

  form: function BTA_form() {
    dbg_assert(!this.exited,
               "form() shouldn't be called on exited browser actor.");
    dbg_assert(this.actorID,
               "tab should have an actorID.");

    let response = {
      actor: this.actorID
    };

    
    
    if (this.window) {
      response.title = this.title;
      response.url = this.url;
      let windowUtils = this.window
        .QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindowUtils);
      response.outerWindowID = windowUtils.outerWindowID;
    }

    
    
    if (!this._tabActorPool) {
      this._tabActorPool = new ActorPool(this.conn);
      this.conn.addActorPool(this._tabActorPool);
    }

    
    
    
    this._createExtraActors(DebuggerServer.tabActorFactories,
      this._tabActorPool);

    this._appendExtraActors(response);
    return response;
  },

  


  disconnect: function BTA_disconnect() {
    this.exit();
  },

  


  exit: function BTA_exit() {
    if (this.exited) {
      return;
    }

    
    
    if (this._attached) {
      this.threadActor._tabClosed = true;
    }

    if (this._detach()) {
      this.conn.send({ from: this.actorID,
                       type: "tabDetached" });
    }

    Object.defineProperty(this, "docShell", {
      value: null,
      configurable: true
    });

    this._extraActors = null;

    this._exited = true;
  },

  



  _shouldAddNewGlobalAsDebuggee: function (wrappedGlobal) {
    if (wrappedGlobal.hostAnnotations &&
        wrappedGlobal.hostAnnotations.type == "document" &&
        wrappedGlobal.hostAnnotations.element === this.window) {
      return true;
    }

    let global = unwrapDebuggerObjectGlobal(wrappedGlobal);
    if (!global) {
      return false;
    }

    
    let metadata = {};
    let id = "";
    try {
      id = getInnerId(this.window);
      metadata = Cu.getSandboxMetadata(global);
    }
    catch (e) {}
    if (metadata
        && metadata["inner-window-id"]
        && metadata["inner-window-id"] == id) {
      return true;
    }

    return false;
  },

  
  _createExtraActors: createExtraActors,
  _appendExtraActors: appendExtraActors,

  


  _attach: function BTA_attach() {
    if (this._attached) {
      return;
    }

    
    dbg_assert(!this._tabPool, "Shouldn't have a tab pool if we weren't attached.");
    this._tabPool = new ActorPool(this.conn);
    this.conn.addActorPool(this._tabPool);

    
    this._pushContext();

    
    if (this.window) {
      this._progressListener = new DebuggerProgressListener(this);

      
      this._originalWindow = this.window;

      
      
      DevToolsUtils.executeSoon(() => this._watchDocshells());
    }

    this._attached = true;
  },

  _watchDocshells: function BTA_watchDocshells() {
    
    if (this.listenForNewDocShells) {
      Services.obs.addObserver(this, "webnavigation-create", false);
    }
    Services.obs.addObserver(this, "webnavigation-destroy", false);

    
    this._progressListener.watch(this.docShell);

    
    this._updateChildDocShells();
  },

  onSwitchToFrame: function BTA_onSwitchToFrame(aRequest) {
    let windowId = aRequest.windowId;
    let win;
    try {
      win = Services.wm.getOuterWindowWithId(windowId);
    } catch(e) {}
    if (!win) {
      return { error: "noWindow",
               message: "The related docshell is destroyed or not found" };
    } else if (win == this.window) {
      return {};
    }

    
    DevToolsUtils.executeSoon(() => this._changeTopLevelDocument(win));

    return {};
  },

  onListFrames: function BTA_onListFrames(aRequest) {
    let windows = this._docShellsToWindows(this.docShells);
    return { frames: windows };
  },

  onListWorkers: function BTA_onListWorkers(aRequest) {
    if (this._workerActorList === null) {
      this._workerActorList = new WorkerActorList({
        type: Ci.nsIWorkerDebugger.TYPE_DEDICATED,
        window: this.window
      });
    }

    return this._workerActorList.getList().then((actors) => {
      let pool = new ActorPool(this.conn);
      for (let actor of actors) {
        pool.addActor(actor);
      }

      this.conn.removeActorPool(this._workerActorPool);
      this._workerActorPool = pool;
      this.conn.addActorPool(this._workerActorPool);

      this._workerActorList.onListChanged = this._onWorkerActorListChanged;

      return {
        "from": this.actorID,
        "workers": actors.map((actor) => actor.form())
      };
    });
  },

  _onWorkerActorListChanged: function () {
    this._workerActorList.onListChanged = null;
    this.conn.sendActorEvent(this.actorID, "workerListChanged");
  },

  observe: function (aSubject, aTopic, aData) {
    
    
    if (!this.attached) {
      return;
    }
    if (aTopic == "webnavigation-create") {
      aSubject.QueryInterface(Ci.nsIDocShell);
      this._onDocShellCreated(aSubject);
    } else if (aTopic == "webnavigation-destroy") {
      this._onDocShellDestroy(aSubject);
    }
  },

  _onDocShellCreated: function (docShell) {
    
    
    
    
    
    
    
    DevToolsUtils.executeSoon(() => {
      
      
      if (this._isRootDocShell(docShell)) {
        this._progressListener.watch(docShell);
      }
      this._notifyDocShellsUpdate([docShell]);
    });
  },

  _onDocShellDestroy: function (docShell) {
    let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebProgress);
    this._notifyDocShellDestroy(webProgress);
  },

  _isRootDocShell: function (docShell) {
    
    
    
    
    
    return !docShell.chromeEventHandler ||
           !(docShell.chromeEventHandler instanceof Ci.nsIDOMElement);
  },

  
  _docShellsToWindows: function (docshells) {
    return docshells.map(docShell => {
      let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIWebProgress);
      let window = webProgress.DOMWindow;
      let id = window.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils)
                     .outerWindowID;
      let parentID = undefined;
      
      
      if (window.parent && window != this._originalWindow) {
        parentID = window.parent
                         .QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils)
                         .outerWindowID;
      }
      return {
        id: id,
        url: window.location.href,
        title: window.document.title,
        parentID: parentID
      };
    });
  },

  _notifyDocShellsUpdate: function (docshells) {
    let windows = this._docShellsToWindows(docshells);
    this.conn.send({ from: this.actorID,
                     type: "frameUpdate",
                     frames: windows
                   });
  },

  _updateChildDocShells: function () {
    this._notifyDocShellsUpdate(this.docShells);
  },

  _notifyDocShellDestroy: function (webProgress) {
    webProgress = webProgress.QueryInterface(Ci.nsIWebProgress);
    let id = webProgress.DOMWindow
                        .QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIDOMWindowUtils)
                        .outerWindowID;
    this.conn.send({ from: this.actorID,
                     type: "frameUpdate",
                     frames: [{
                       id: id,
                       destroy: true
                     }]
                   });

    
    
    webProgress.QueryInterface(Ci.nsIDocShell);
    if (this._isRootDocShell(webProgress)) {
      this._progressListener.unwatch(webProgress);
    }

    if (webProgress.DOMWindow == this._originalWindow) {
      
      
      let rootDocShells = this.docShells
                              .filter(d => {
                                return d != this.docShell &&
                                       this._isRootDocShell(d);
                              });
      if (rootDocShells.length > 0) {
        let newRoot = rootDocShells[0];
        this._originalWindow = newRoot.DOMWindow;
        this._changeTopLevelDocument(this._originalWindow);
      } else {
        
        
        
        
        this.exit();
      }
      return;
    }

    
    
    
    if (webProgress.DOMWindow == this.window &&
        this.window != this._originalWindow) {
      this._changeTopLevelDocument(this._originalWindow);
    }
  },

  _notifyDocShellDestroyAll: function () {
    this.conn.send({ from: this.actorID,
                     type: "frameUpdate",
                     destroyAll: true
                   });
  },

  



  _pushContext: function BTA_pushContext() {
    dbg_assert(!this._contextPool, "Can't push multiple contexts");

    this._contextPool = new ActorPool(this.conn);
    this.conn.addActorPool(this._contextPool);

    this.threadActor = new ThreadActor(this, this.window);
    this._contextPool.addActor(this.threadActor);
  },

  



  _popContext: function BTA_popContext() {
    dbg_assert(!!this._contextPool, "No context to pop.");

    this.conn.removeActorPool(this._contextPool);
    this._contextPool = null;
    this.threadActor.exit();
    this.threadActor = null;
    this._sources = null;
  },

  




  _detach: function BTA_detach() {
    if (!this.attached) {
      return false;
    }

    
    
    if (this.docShell) {
      this._progressListener.unwatch(this.docShell);
      this._restoreDocumentSettings();
    }
    if (this._progressListener) {
      this._progressListener.destroy();
      this._progressListener = null;
      this._originalWindow = null;

      
      if (this.listenForNewDocShells) {
        Services.obs.removeObserver(this, "webnavigation-create");
      }
      Services.obs.removeObserver(this, "webnavigation-destroy");
    }

    this._popContext();

    
    for (let sheetActor of this._styleSheetActors.values()) {
      this._tabPool.removeActor(sheetActor);
    }
    this._styleSheetActors.clear();
    this.conn.removeActorPool(this._tabPool);
    this._tabPool = null;
    if (this._tabActorPool) {
      this.conn.removeActorPool(this._tabActorPool);
      this._tabActorPool = null;
    }

    this._attached = false;
    return true;
  },

  

  onAttach: function BTA_onAttach(aRequest) {
    if (this.exited) {
      return { type: "exited" };
    }

    this._attach();

    return {
      type: "tabAttached",
      threadActor: this.threadActor.actorID,
      cacheDisabled: this._getCacheDisabled(),
      javascriptEnabled: this._getJavascriptEnabled(),
      traits: this.traits,
    };
  },

  onDetach: function BTA_onDetach(aRequest) {
    if (!this._detach()) {
      return { error: "wrongState" };
    }

    return { type: "detached" };
  },

  


  onReload: function(aRequest) {
    let force = aRequest && aRequest.options && aRequest.options.force;
    
    
    Services.tm.currentThread.dispatch(DevToolsUtils.makeInfallible(() => {
      
      
      if (Services.startup.shuttingDown) {
        return;
      }
      this.webNavigation.reload(force ? Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE
                                      : Ci.nsIWebNavigation.LOAD_FLAGS_NONE);
    }, "TabActor.prototype.onReload's delayed body"), 0);
    return {};
  },

  


  onNavigateTo: function(aRequest) {
    
    
    Services.tm.currentThread.dispatch(DevToolsUtils.makeInfallible(() => {
      this.window.location = aRequest.url;
    }, "TabActor.prototype.onNavigateTo's delayed body"), 0);
    return {};
  },

  


  onReconfigure: function (aRequest) {
    let options = aRequest.options || {};

    if (!this.docShell) {
      
      return {};
    }
    this._toggleDevToolsSettings(options);

    return {};
  },

  


  _toggleDevToolsSettings: function(options) {
    
    
    let reload = false;

    if (typeof options.javascriptEnabled !== "undefined" &&
        options.javascriptEnabled !== this._getJavascriptEnabled()) {
      this._setJavascriptEnabled(options.javascriptEnabled);
      reload = true;
    }
    if (typeof options.cacheDisabled !== "undefined" &&
        options.cacheDisabled !== this._getCacheDisabled()) {
      this._setCacheDisabled(options.cacheDisabled);
    }
    if ((typeof options.serviceWorkersTestingEnabled !== "undefined") &&
        (options.serviceWorkersTestingEnabled !==
         this._getServiceWorkersTestingEnabled())) {
      this._setServiceWorkersTestingEnabled(
        options.serviceWorkersTestingEnabled
      );
    }

    
    
    
    let hasExplicitReloadFlag = "performReload" in options;
    if ((hasExplicitReloadFlag && options.performReload) ||
       (!hasExplicitReloadFlag && reload)) {
      this.onReload();
    }
  },

  



  _restoreDocumentSettings: function () {
    this._restoreJavascript();
    this._setCacheDisabled(false);
    this._setServiceWorkersTestingEnabled(false);
  },

  


  _setCacheDisabled: function(disabled) {
    let enable =  Ci.nsIRequest.LOAD_NORMAL;
    let disable = Ci.nsIRequest.LOAD_BYPASS_CACHE |
                  Ci.nsIRequest.INHIBIT_CACHING;

    this.docShell.defaultLoadFlags = disabled ? disable : enable;
  },

  


  _wasJavascriptEnabled: null,
  _setJavascriptEnabled: function(allow) {
    if (this._wasJavascriptEnabled === null) {
      this._wasJavascriptEnabled = this.docShell.allowJavascript;
    }
    this.docShell.allowJavascript = allow;
  },

  


  _restoreJavascript: function () {
    if (this._wasJavascriptEnabled !== null) {
      this._setJavascriptEnabled(this._wasJavascriptEnabled);
      this._wasJavascriptEnabled = null;
    }
  },

  


  _getJavascriptEnabled: function() {
    if (!this.docShell) {
      
      return null;
    }

    return this.docShell.allowJavascript;
  },

  


  _setServiceWorkersTestingEnabled: function(enabled) {
    let windowUtils = this.window.QueryInterface(Ci.nsIInterfaceRequestor)
                                 .getInterface(Ci.nsIDOMWindowUtils);
    windowUtils.serviceWorkersTestingEnabled = enabled;
  },

  


  _getCacheDisabled: function() {
    if (!this.docShell) {
      
      return null;
    }

    let disable = Ci.nsIRequest.LOAD_BYPASS_CACHE |
                  Ci.nsIRequest.INHIBIT_CACHING;
    return this.docShell.defaultLoadFlags === disable;
  },

  


  _getServiceWorkersTestingEnabled: function() {
    if (!this.docShell) {
      
      return null;
    }

    let windowUtils = this.window.QueryInterface(Ci.nsIInterfaceRequestor)
                                 .getInterface(Ci.nsIDOMWindowUtils);
    return windowUtils.serviceWorkersTestingEnabled;
  },

  


  preNest: function BTA_preNest() {
    if (!this.window) {
      
      return;
    }
    let windowUtils = this.window
                          .QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);
    windowUtils.suppressEventHandling(true);
    windowUtils.suspendTimeouts();
  },

  


  postNest: function BTA_postNest(aNestData) {
    if (!this.window) {
      
      return;
    }
    let windowUtils = this.window
                          .QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);
    windowUtils.resumeTimeouts();
    windowUtils.suppressEventHandling(false);
    if (this._pendingNavigation) {
      this._pendingNavigation.resume();
      this._pendingNavigation = null;
    }
  },

  _changeTopLevelDocument: function (window) {
    
    
    this._willNavigate(this.window, window.location.href, null, true);

    this._windowDestroyed(this.window, null, true);

    
    
    this._setWindow(window);

    DevToolsUtils.executeSoon(() => {
      
      this._windowReady(window, true);
      DevToolsUtils.executeSoon(() => {
        this._navigate(window, true);
      });
    });
  },

  _setWindow: function (window) {
    let docShell = window.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIWebNavigation)
                         .QueryInterface(Ci.nsIDocShell);
    
    
    
    Object.defineProperty(this, "docShell", {
      value: docShell,
      enumerable: true,
      configurable: true
    });
    events.emit(this, "changed-toplevel-document");
    let id = window.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindowUtils)
                   .outerWindowID;
    this.conn.send({ from: this.actorID,
                     type: "frameUpdate",
                     selected: id
                   });
  },

  




  _windowReady: function (window, isFrameSwitching = false) {
    let isTopLevel = window == this.window;

    
    
    if (window == this._originalWindow && !isFrameSwitching) {
      this._updateChildDocShells();
    }

    events.emit(this, "window-ready", {
      window: window,
      isTopLevel: isTopLevel,
      id: getWindowID(window)
    });

    
    let threadActor = this.threadActor;
    if (isTopLevel && threadActor.state != "detached") {
      this.sources.reset({ sourceMaps: true });
      threadActor.clearDebuggees();
      threadActor.dbg.enabled = true;
      threadActor.maybePauseOnExceptions();
      
      
      threadActor.global = window;
    }

    
    
    if (threadActor.attached) {
      threadActor.dbg.addDebuggees();
    }
  },

  _windowDestroyed: function (window, id = null, isFrozen = false) {
    events.emit(this, "window-destroyed", {
      window: window,
      isTopLevel: window == this.window,
      id: id || getWindowID(window),
      isFrozen: isFrozen
    });
  },

  



  _willNavigate: function (window, newURI, request, isFrameSwitching = false) {
    let isTopLevel = window == this.window;
    let reset = false;

    if (window == this._originalWindow && !isFrameSwitching) {
      
      this._notifyDocShellDestroyAll();

      
      
      
      
      
      if (this.window != this._originalWindow) {
        reset=true;
        window = this.window;
        isTopLevel = true;
      }
    }

    
    
    
    
    
    events.emit(this, "will-navigate", {
      window: window,
      isTopLevel: isTopLevel,
      newURI: newURI,
      request: request
    });

    
    
    if (!isTopLevel) {
      return;
    }

    
    
    let threadActor = this.threadActor;
    if (request && threadActor.state == "paused") {
      request.suspend();
      this.conn.send(threadActor.synchronize(Promise.resolve(threadActor.onResume())));
      threadActor.dbg.enabled = false;
      this._pendingNavigation = request;
    }
    threadActor.disableAllBreakpoints();

    this.conn.send({
      from: this.actorID,
      type: "tabNavigated",
      url: newURI,
      nativeConsoleAPI: true,
      state: "start",
      isFrameSwitching: isFrameSwitching
    });

    if (reset) {
      this._setWindow(this._originalWindow);
    }
  },

  



  _navigate: function (window, isFrameSwitching = false) {
    let isTopLevel = window == this.window;

    
    
    
    
    events.emit(this, "navigate", {
      window: window,
      isTopLevel: isTopLevel
    });

    
    
    if (!isTopLevel) {
      return;
    }

    
    let threadActor = this.threadActor;
    if (threadActor.state == "running") {
      threadActor.dbg.enabled = true;
    }

    this.conn.send({
      from: this.actorID,
      type: "tabNavigated",
      url: this.url,
      title: this.title,
      nativeConsoleAPI: this.hasNativeConsoleAPI(this.window),
      state: "stop",
      isFrameSwitching: isFrameSwitching
    });
  },

  








  hasNativeConsoleAPI: function BTA_hasNativeConsoleAPI(aWindow) {
    let isNative = false;
    try {
      
      
      let console = aWindow.wrappedJSObject.console;
      isNative = console instanceof aWindow.Console;
    }
    catch (ex) { }
    return isNative;
  },

  









  createStyleSheetActor: function BTA_createStyleSheetActor(styleSheet) {
    if (this._styleSheetActors.has(styleSheet)) {
      return this._styleSheetActors.get(styleSheet);
    }
    let actor = new StyleSheetActor(styleSheet, this);
    this._styleSheetActors.set(styleSheet, actor);

    this._tabPool.addActor(actor);

    return actor;
  },

  removeActorByName: function BTA_removeActor(aName) {
    if (aName in this._extraActors) {
      const actor = this._extraActors[aName];
      if (this._tabActorPool.has(actor)) {
        this._tabActorPool.removeActor(actor);
      }
      delete this._extraActors[aName];
    }
  },
};




TabActor.prototype.requestTypes = {
  "attach": TabActor.prototype.onAttach,
  "detach": TabActor.prototype.onDetach,
  "reload": TabActor.prototype.onReload,
  "navigateTo": TabActor.prototype.onNavigateTo,
  "reconfigure": TabActor.prototype.onReconfigure,
  "switchToFrame": TabActor.prototype.onSwitchToFrame,
  "listFrames": TabActor.prototype.onListFrames,
  "listWorkers": TabActor.prototype.onListWorkers
};

exports.TabActor = TabActor;












function BrowserTabActor(aConnection, aBrowser, aTabBrowser)
{
  TabActor.call(this, aConnection, aBrowser);
  this._browser = aBrowser;
  this._tabbrowser = aTabBrowser;

  Object.defineProperty(this, "docShell", {
    value: this._browser.docShell,
    configurable: true
  });
}

BrowserTabActor.prototype = Object.create(TabActor.prototype);

BrowserTabActor.prototype.constructor = BrowserTabActor;

Object.defineProperty(BrowserTabActor.prototype, "title", {
  get: function() {
    
    if (this._browser.__SS_restore) {
      let sessionStore = this._browser.__SS_data;
      
      let entry = sessionStore.entries[sessionStore.index - 1];
      return entry.title;
    }
    let title = this.contentDocument.title || this._browser.contentTitle;
    
    
    
    if (!title && this._tabbrowser) {
      let tab = this._tabbrowser._getTabForContentWindow(this.window);
      if (tab) {
        title = tab.label;
      }
    }
    return title;
  },
  enumerable: true,
  configurable: false
});

Object.defineProperty(BrowserTabActor.prototype, "url", {
  get: function() {
    
    if (this._browser.__SS_restore) {
      let sessionStore = this._browser.__SS_data;
      
      let entry = sessionStore.entries[sessionStore.index - 1];
      return entry.url;
    }
    if (this.webNavigation.currentURI) {
      return this.webNavigation.currentURI.spec;
    }
    return null;
  },
  enumerable: true,
  configurable: true
});

Object.defineProperty(BrowserTabActor.prototype, "browser", {
  get: function() {
    return this._browser;
  },
  enumerable: true,
  configurable: false
});

BrowserTabActor.prototype.disconnect = function() {
  TabActor.prototype.disconnect.call(this);
  this._browser = null;
  this._tabbrowser = null;
};

BrowserTabActor.prototype.exit = function() {
  TabActor.prototype.exit.call(this);
  this._browser = null;
  this._tabbrowser = null;
};

exports.BrowserTabActor = BrowserTabActor;









function RemoteBrowserTabActor(aConnection, aBrowser)
{
  this._conn = aConnection;
  this._browser = aBrowser;
  this._form = null;
}

RemoteBrowserTabActor.prototype = {
  connect: function() {
    let onDestroy = () => {
      this._form = null;
    };
    let connect = DebuggerServer.connectToChild(this._conn, this._browser, onDestroy);
    return connect.then(form => {
      this._form = form;
      return this;
    });
  },

  get _mm() {
    return this._browser.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader
           .messageManager;
  },

  update: function() {
    
    
    if (this._form) {
      let deferred = promise.defer();
      let onFormUpdate = msg => {
        this._mm.removeMessageListener("debug:form", onFormUpdate);
        this._form = msg.json;
        deferred.resolve(this);
      };
      this._mm.addMessageListener("debug:form", onFormUpdate);
      this._mm.sendAsyncMessage("debug:form");
      return deferred.promise;
    } else {
      return this.connect();
    }
  },

  form: function() {
    return this._form;
  },

  exit: function() {
    this._browser = null;
  },
};

exports.RemoteBrowserTabActor = RemoteBrowserTabActor;

function BrowserAddonList(aConnection)
{
  this._connection = aConnection;
  this._actorByAddonId = new Map();
  this._onListChanged = null;
}

BrowserAddonList.prototype.getList = function() {
  var deferred = promise.defer();
  AddonManager.getAllAddons((addons) => {
    for (let addon of addons) {
      let actor = this._actorByAddonId.get(addon.id);
      if (!actor) {
        actor = new BrowserAddonActor(this._connection, addon);
        this._actorByAddonId.set(addon.id, actor);
      }
    }
    deferred.resolve([...this._actorByAddonId].map(([_, actor]) => actor));
  });
  return deferred.promise;
}

Object.defineProperty(BrowserAddonList.prototype, "onListChanged", {
  enumerable: true, configurable: true,
  get: function() { return this._onListChanged; },
  set: function(v) {
    if (v !== null && typeof v != "function") {
      throw Error("onListChanged property may only be set to 'null' or a function");
    }
    this._onListChanged = v;
    if (this._onListChanged) {
      AddonManager.addAddonListener(this);
    } else {
      AddonManager.removeAddonListener(this);
    }
  }
});

BrowserAddonList.prototype.onInstalled = function (aAddon) {
  this._onListChanged();
};

BrowserAddonList.prototype.onUninstalled = function (aAddon) {
  this._actorByAddonId.delete(aAddon.id);
  this._onListChanged();
};

exports.BrowserAddonList = BrowserAddonList;










function DebuggerProgressListener(aTabActor) {
  this._tabActor = aTabActor;
  this._onWindowCreated = this.onWindowCreated.bind(this);
  this._onWindowHidden = this.onWindowHidden.bind(this);

  
  Services.obs.addObserver(this, "inner-window-destroyed", false);

  
  
  
  this._knownWindowIDs = new Map();
}

DebuggerProgressListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIWebProgressListener,
    Ci.nsISupportsWeakReference,
    Ci.nsISupports,
  ]),

  destroy: function() {
    Services.obs.removeObserver(this, "inner-window-destroyed", false);
    this._knownWindowIDs.clear();
    this._knownWindowIDs = null;
  },

  watch: function(docShell) {
    let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebProgress);
    webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_STATUS |
                                          Ci.nsIWebProgress.NOTIFY_STATE_WINDOW |
                                          Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT);

    let handler = getDocShellChromeEventHandler(docShell);
    handler.addEventListener("DOMWindowCreated", this._onWindowCreated, true);
    handler.addEventListener("pageshow", this._onWindowCreated, true);
    handler.addEventListener("pagehide", this._onWindowHidden, true);

    
    for (let win of this._getWindowsInDocShell(docShell)) {
      this._tabActor._windowReady(win);
      this._knownWindowIDs.set(getWindowID(win), win);
    }
  },

  unwatch: function(docShell) {
    let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebProgress);
    
    try {
      webProgress.removeProgressListener(this);
    } catch(e) {}

    let handler = getDocShellChromeEventHandler(docShell);
    handler.removeEventListener("DOMWindowCreated", this._onWindowCreated, true);
    handler.removeEventListener("pageshow", this._onWindowCreated, true);
    handler.removeEventListener("pagehide", this._onWindowHidden, true);

    for (let win of this._getWindowsInDocShell(docShell)) {
      this._knownWindowIDs.delete(getWindowID(win));
    }
  },

  _getWindowsInDocShell: function(docShell) {
    return getChildDocShells(docShell).map(d => {
      return d.QueryInterface(Ci.nsIInterfaceRequestor)
              .getInterface(Ci.nsIDOMWindow);
    });
  },

  onWindowCreated: DevToolsUtils.makeInfallible(function(evt) {
    if (!this._tabActor.attached) {
      return;
    }

    
    
    
    if (evt.type == "pageshow" && !evt.persisted) {
      return;
    }

    let window = evt.target.defaultView;
    this._tabActor._windowReady(window);

    if (evt.type !== "pageshow") {
      this._knownWindowIDs.set(getWindowID(window), window);
    }
  }, "DebuggerProgressListener.prototype.onWindowCreated"),

  onWindowHidden: DevToolsUtils.makeInfallible(function(evt) {
    if (!this._tabActor.attached) {
      return;
    }

    
    
    
    
    if (!evt.persisted) {
      return;
    }

    let window = evt.target.defaultView;
    this._tabActor._windowDestroyed(window, null, true);
  }, "DebuggerProgressListener.prototype.onWindowHidden"),

  observe: DevToolsUtils.makeInfallible(function(subject, topic) {
    if (!this._tabActor.attached) {
      return;
    }

    
    
    
    let innerID = subject.QueryInterface(Ci.nsISupportsPRUint64).data;
    let window = this._knownWindowIDs.get(innerID);
    if (window) {
      this._knownWindowIDs.delete(innerID);
      this._tabActor._windowDestroyed(window, innerID);
    }
  }, "DebuggerProgressListener.prototype.observe"),

  onStateChange:
  DevToolsUtils.makeInfallible(function(aProgress, aRequest, aFlag, aStatus) {
    if (!this._tabActor.attached) {
      return;
    }

    let isStart = aFlag & Ci.nsIWebProgressListener.STATE_START;
    let isStop = aFlag & Ci.nsIWebProgressListener.STATE_STOP;
    let isDocument = aFlag & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT;
    let isWindow = aFlag & Ci.nsIWebProgressListener.STATE_IS_WINDOW;

    
    if (isDocument && isStop) {
      
      aProgress.QueryInterface(Ci.nsIDocShell);
      this._tabActor._notifyDocShellsUpdate([aProgress]);
    }

    let window = aProgress.DOMWindow;
    if (isDocument && isStart) {
      
      
      let newURI = aRequest instanceof Ci.nsIChannel ? aRequest.URI.spec : null;
      this._tabActor._willNavigate(window, newURI, aRequest);
    }
    if (isWindow && isStop) {
      
      
      this._tabActor._navigate(window);
    }
  }, "DebuggerProgressListener.prototype.onStateChange")
};

exports.register = function(handle) {
  handle.setRootActor(createRootActor);
};

exports.unregister = function(handle) {
  handle.setRootActor(null);
};
