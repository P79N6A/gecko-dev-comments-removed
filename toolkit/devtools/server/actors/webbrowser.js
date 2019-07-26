





"use strict";

let promise = Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js", {}).Promise;
XPCOMUtils.defineLazyModuleGetter(this, "AddonManager", "resource://gre/modules/AddonManager.jsm");










function allAppShellDOMWindows(aWindowType)
{
  let e = Services.wm.getEnumerator(aWindowType);
  while (e.hasMoreElements()) {
    yield e.getNext();
  }
}




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

BrowserTabList.prototype._getChildren = function(aWindow) {
  return aWindow.gBrowser.browsers;
};

BrowserTabList.prototype.getList = function() {
  let topXULWindow = Services.wm.getMostRecentWindow(DebuggerServer.chromeWindowType);

  
  
  let initialMapSize = this._actorByBrowser.size;
  let foundCount = 0;

  
  
  
  

  let actorPromises = [];

  
  for (let win of allAppShellDOMWindows(DebuggerServer.chromeWindowType)) {
    let selectedBrowser = this._getSelectedBrowser(win);
    if (!selectedBrowser) {
      continue;
    }

    
    
    
    
    for (let browser of this._getChildren(win)) {
      
      let actor = this._actorByBrowser.get(browser);
      if (actor) {
        actorPromises.push(promise.resolve(actor));
        foundCount++;
      } else if (browser.isRemoteBrowser) {
        actor = new RemoteBrowserTabActor(this._connection, browser);
        this._actorByBrowser.set(browser, actor);
        let promise = actor.connect().then((form) => {
          actor._form = form;
          return actor;
        });
        actorPromises.push(promise);
      } else {
        actor = new BrowserTabActor(this._connection, browser, win.gBrowser);
        this._actorByBrowser.set(browser, actor);
        actorPromises.push(promise.resolve(actor));
      }

      
      actor.selected = (win === topXULWindow && browser === selectedBrowser);
    }
  }

  if (this._testing && initialMapSize !== foundCount)
    throw Error("_actorByBrowser map contained actors for dead tabs");

  this._mustNotify = true;
  this._checkListening();

  return promise.all(actorPromises);
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















function TabActor(aConnection, aChromeEventHandler)
{
  this.conn = aConnection;
  this._chromeEventHandler = aChromeEventHandler;
  this._tabActorPool = null;
  
  this._extraActors = {};

  this._onWindowCreated = this.onWindowCreated.bind(this);

  this.traits = { reconfigure: true };
}




TabActor.prototype = {
  traits: null,

  get exited() { return !this._chromeEventHandler; },
  get attached() { return !!this._attached; },

  _tabPool: null,
  get tabActorPool() { return this._tabPool; },

  _contextPool: null,
  get contextActorPool() { return this._contextPool; },

  _pendingNavigation: null,

  
  actorPrefix: "tab",

  


  get chromeEventHandler() {
    return this._chromeEventHandler;
  },

  


  get docShell() {
    throw "The docShell getter should be implemented by a subclass of TabActor";
  },

  


  get window() {
    return this.docShell
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindow);
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

  form: function BTA_form() {
    dbg_assert(!this.exited,
               "grip() shouldn't be called on exited browser actor.");
    dbg_assert(this.actorID,
               "tab should have an actorID.");

    let windowUtils = this.window
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindowUtils);

    let response = {
      actor: this.actorID,
      title: this.title,
      url: this.url,
      outerWindowID: windowUtils.outerWindowID
    };

    
    let actorPool = new ActorPool(this.conn);
    this._createExtraActors(DebuggerServer.tabActorFactories, actorPool);
    if (!actorPool.isEmpty()) {
      this._tabActorPool = actorPool;
      this.conn.addActorPool(this._tabActorPool);
    }

    this._appendExtraActors(response);
    return response;
  },

  


  disconnect: function BTA_disconnect() {
    this._detach();
    this._extraActors = null;
    this._chromeEventHandler = null;
  },

  


  exit: function BTA_exit() {
    if (this.exited) {
      return;
    }

    if (this._detach()) {
      this.conn.send({ from: this.actorID,
                       type: "tabDetached" });
    }

    this._chromeEventHandler = null;
  },

  
  _createExtraActors: CommonCreateExtraActors,
  _appendExtraActors: CommonAppendExtraActors,

  


  _attach: function BTA_attach() {
    if (this._attached) {
      return;
    }

    
    dbg_assert(!this._tabPool, "Shouldn't have a tab pool if we weren't attached.");
    this._tabPool = new ActorPool(this.conn);
    this.conn.addActorPool(this._tabPool);

    
    this._pushContext();

    
    this.chromeEventHandler.addEventListener("DOMWindowCreated", this._onWindowCreated, true);
    this.chromeEventHandler.addEventListener("pageshow", this._onWindowCreated, true);
    this._progressListener = new DebuggerProgressListener(this);

    this._attached = true;
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
  },

  




  _detach: function BTA_detach() {
    if (!this.attached) {
      return false;
    }

    this._progressListener.destroy();

    this.chromeEventHandler.removeEventListener("DOMWindowCreated", this._onWindowCreated, true);
    this.chromeEventHandler.removeEventListener("pageshow", this._onWindowCreated, true);

    this._popContext();

    
    this.conn.removeActorPool(this._tabPool);
    this._tabPool = null;
    if (this._tabActorPool) {
      this.conn.removeActorPool(this._tabActorPool, true);
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
      cacheEnabled: this._getCacheEnabled(),
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
    
    
    Services.tm.currentThread.dispatch(DevToolsUtils.makeInfallible(() => {
      this.window.location.reload();
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

    this._toggleJsOrCache(options);
    return {};
  },

  


  _toggleJsOrCache: function(options) {
    
    
    let reload = false;

    if (typeof options.javascriptEnabled !== "undefined" &&
        options.javascriptEnabled !== this._getJavascriptEnabled()) {
      this._setJavascriptEnabled(options.javascriptEnabled);
      reload = true;
    }
    if (typeof options.cacheEnabled !== "undefined" &&
        options.cacheEnabled !== this._getCacheEnabled()) {
      this._setCacheEnabled(options.cacheEnabled);
      reload = true;
    }

    
    
    
    let hasExplicitReloadFlag = "performReload" in options;
    if ((hasExplicitReloadFlag && options.performReload) ||
       (!hasExplicitReloadFlag && reload)) {
      this.onReload();
    }
  },

  


  _setCacheEnabled: function(allow) {
    let enable =  Ci.nsIRequest.LOAD_NORMAL;
    let disable = Ci.nsIRequest.LOAD_BYPASS_CACHE |
                  Ci.nsIRequest.INHIBIT_CACHING;
    if (this.docShell) {
      this.docShell.defaultLoadFlags = allow ? enable : disable;
    }
  },

  


  _setJavascriptEnabled: function(allow) {
    if (this.docShell) {
      this.docShell.allowJavascript = allow;
    }
  },

  


  _getCacheEnabled: function() {
    if (!this.docShell) {
      
      return null;
    }

    let disable = Ci.nsIRequest.LOAD_BYPASS_CACHE |
                  Ci.nsIRequest.INHIBIT_CACHING;
    return this.docShell.defaultLoadFlags !== disable;
  },

  


  _getJavascriptEnabled: function() {
    if (!this.docShell) {
      
      return null;
    }

    return this.docShell.allowJavascript;
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

  




  onWindowCreated:
  DevToolsUtils.makeInfallible(function BTA_onWindowCreated(evt) {
    
    
    if (!this._attached || (evt.type == "pageshow" && !evt.persisted)) {
      return;
    }
    if (evt.target === this.contentDocument) {
      this.threadActor.clearDebuggees();
      if (this.threadActor.dbg) {
        this.threadActor.dbg.enabled = true;
        this.threadActor.global = evt.target.defaultView;
        this.threadActor.maybePauseOnExceptions();
      }
    }

    
    
    if (this.threadActor.attached) {
      this.threadActor.findGlobals();
    }
  }, "TabActor.prototype.onWindowCreated"),

  








  hasNativeConsoleAPI: function BTA_hasNativeConsoleAPI(aWindow) {
    
    
    return WebConsoleActor.prototype.hasNativeConsoleAPI(aWindow);
  }
};




TabActor.prototype.requestTypes = {
  "attach": TabActor.prototype.onAttach,
  "detach": TabActor.prototype.onDetach,
  "reload": TabActor.prototype.onReload,
  "navigateTo": TabActor.prototype.onNavigateTo,
  "reconfigure": TabActor.prototype.onReconfigure
};












function BrowserTabActor(aConnection, aBrowser, aTabBrowser)
{
  TabActor.call(this, aConnection, aBrowser);
  this._browser = aBrowser;
  this._tabbrowser = aTabBrowser;
}

BrowserTabActor.prototype = Object.create(TabActor.prototype);

BrowserTabActor.prototype.constructor = BrowserTabActor;

Object.defineProperty(BrowserTabActor.prototype, "docShell", {
  get: function() {
    return this._browser.docShell;
  },
  enumerable: true,
  configurable: false
});

Object.defineProperty(BrowserTabActor.prototype, "title", {
  get: function() {
    let title = this.contentDocument.contentTitle;
    
    
    
    if (!title && this._tabbrowser) {
      title = this._tabbrowser._getTabForContentWindow(this.window).label;
    }
    return title;
  },
  enumerable: true,
  configurable: false
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









function RemoteBrowserTabActor(aConnection, aBrowser)
{
  this._conn = aConnection;
  this._browser = aBrowser;
  this._form = null;
}

RemoteBrowserTabActor.prototype = {
  connect: function() {
    return DebuggerServer.connectToChild(this._conn, this._browser.messageManager);
  },

  form: function() {
    return this._form;
  },

  exit: function() {
    this._browser = null;
  },
};

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
    deferred.resolve([actor for ([_, actor] of this._actorByAddonId)]);
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

function BrowserAddonActor(aConnection, aAddon) {
  this.conn = aConnection;
  this._addon = aAddon;
  this._contextPool = null;
  this._threadActor = null;
  AddonManager.addAddonListener(this);
}

BrowserAddonActor.prototype = {
  actorPrefix: "addon",

  get exited() {
    return !this._addon;
  },

  get id() {
    return this._addon.id;
  },

  get url() {
    return this._addon.sourceURI ? this._addon.sourceURI.spec : undefined;
  },

  get attached() {
    return this._threadActor;
  },

  form: function BAA_form() {
    dbg_assert(this.actorID, "addon should have an actorID.");

    return {
      actor: this.actorID,
      id: this.id,
      url: this.url
    };
  },

  disconnect: function BAA_disconnect() {
    AddonManager.removeAddonListener(this);
  },

  onUninstalled: function BAA_onUninstalled(aAddon) {
    if (aAddon != this._addon)
      return;

    if (this.attached) {
      this.onDetach();
      this.conn.send({ from: this.actorID, type: "tabDetached" });
    }

    this._addon = null;
    AddonManager.removeAddonListener(this);
  },

  onAttach: function BAA_onAttach() {
    if (this.exited) {
      return { type: "exited" };
    }

    if (!this.attached) {
      this._contextPool = new ActorPool(this.conn);
      this.conn.addActorPool(this._contextPool);

      this._threadActor = new AddonThreadActor(this.conn, this,
                                               this._addon.id);
      this._contextPool.addActor(this._threadActor);
    }

    return { type: "tabAttached", threadActor: this._threadActor.actorID };
  },

  onDetach: function BAA_onDetach() {
    if (!this.attached) {
      return { error: "wrongState" };
    }

    this.conn.removeActorPool(this._contextPool);
    this._contextPool = null;

    this._threadActor = null;

    return { type: "detached" };
  },

  preNest: function() {
    let e = Services.wm.getEnumerator(null);
    while (e.hasMoreElements()) {
      let win = e.getNext();
      let windowUtils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      windowUtils.suppressEventHandling(true);
      windowUtils.suspendTimeouts();
    }
  },

  postNest: function() {
    let e = Services.wm.getEnumerator(null);
    while (e.hasMoreElements()) {
      let win = e.getNext();
      let windowUtils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      windowUtils.resumeTimeouts();
      windowUtils.suppressEventHandling(false);
    }
  }
};

BrowserAddonActor.prototype.requestTypes = {
  "attach": BrowserAddonActor.prototype.onAttach,
  "detach": BrowserAddonActor.prototype.onDetach
};










function DebuggerProgressListener(aTabActor) {
  this._tabActor = aTabActor;
  this._tabActor.webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_STATE_ALL);
  let EventEmitter = devtools.require("devtools/toolkit/event-emitter");
  EventEmitter.decorate(this);
}

DebuggerProgressListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIWebProgressListener,
    Ci.nsISupportsWeakReference,
    Ci.nsISupports,
  ]),

  onStateChange:
  DevToolsUtils.makeInfallible(function DPL_onStateChange(aProgress, aRequest, aFlag, aStatus) {
    let isStart = aFlag & Ci.nsIWebProgressListener.STATE_START;
    let isStop = aFlag & Ci.nsIWebProgressListener.STATE_STOP;
    let isDocument = aFlag & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT;
    let isNetwork = aFlag & Ci.nsIWebProgressListener.STATE_IS_NETWORK;
    let isRequest = aFlag & Ci.nsIWebProgressListener.STATE_IS_REQUEST;
    let isWindow = aFlag & Ci.nsIWebProgressListener.STATE_IS_WINDOW;

    
    if (!isWindow || !isNetwork ||
        aProgress.DOMWindow != this._tabActor.window) {
      return;
    }

    if (isStart && aRequest instanceof Ci.nsIChannel) {
      
      if (this._tabActor.threadActor.state == "paused") {
        aRequest.suspend();
        this._tabActor.threadActor.onResume();
        this._tabActor.threadActor.dbg.enabled = false;
        this._tabActor._pendingNavigation = aRequest;
      }

      let packet = {
        from: this._tabActor.actorID,
        type: "tabNavigated",
        url: aRequest.URI.spec,
        nativeConsoleAPI: true,
        state: "start"
      };
      this._tabActor.threadActor.disableAllBreakpoints();
      this._tabActor.conn.send(packet);
      this.emit("will-navigate", packet);
    } else if (isStop) {
      if (this._tabActor.threadActor.state == "running") {
        this._tabActor.threadActor.dbg.enabled = true;
      }

      let window = this._tabActor.window;
      let packet = {
        from: this._tabActor.actorID,
        type: "tabNavigated",
        url: this._tabActor.url,
        title: this._tabActor.title,
        nativeConsoleAPI: this._tabActor.hasNativeConsoleAPI(window),
        state: "stop"
      };
      this._tabActor.conn.send(packet);
      this.emit("navigate", packet);
    }
  }, "DebuggerProgressListener.prototype.onStateChange"),

  


  destroy: function DPL_destroy() {
    try {
      this._tabActor.webProgress.removeProgressListener(this);
    } catch (ex) {
      
    }

    this._tabActor._progressListener = null;
    this._tabActor = null;
  }
};
