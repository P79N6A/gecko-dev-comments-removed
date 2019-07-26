





"use strict";




var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
  .getService(Ci.nsIWindowMediator);

function createRootActor(aConnection)
{
  return new BrowserRootActor(aConnection);
}










function BrowserRootActor(aConnection)
{
  this.conn = aConnection;
  this._tabActors = new WeakMap();
  this._tabActorPool = null;
  
  this._extraActors = {};

  this.onTabClosed = this.onTabClosed.bind(this);
  windowMediator.addListener(this);
}

BrowserRootActor.prototype = {

  


  sayHello: function BRA_sayHello() {
    return {
      from: "root",
      applicationType: "browser",
      traits: {
        sources: true
      }
    };
  },

  


  disconnect: function BRA_disconnect() {
    windowMediator.removeListener(this);
    this._extraActors = null;

    
    
    let e = windowMediator.getEnumerator("navigator:browser");
    while (e.hasMoreElements()) {
      let win = e.getNext();
      this.unwatchWindow(win);
      
      let evt = win.document.createEvent("Event");
      evt.initEvent("Debugger:Shutdown", true, false);
      win.document.documentElement.dispatchEvent(evt);
    }
  },

  




  onListTabs: function BRA_onListTabs() {
    
    

    let actorPool = new ActorPool(this.conn);
    let tabActorList = [];

    
    let actor = this._chromeDebugger;
    if (!actor) {
      actor = new ChromeDebuggerActor(this);
      actor.parentID = this.actorID;
      this._chromeDebugger = actor;
      actorPool.addActor(actor);
    }

    
    let e = windowMediator.getEnumerator("navigator:browser");
    let top = windowMediator.getMostRecentWindow("navigator:browser");
    let selected;
    while (e.hasMoreElements()) {
      let win = e.getNext();

      
      this.watchWindow(win);

      
      let selectedBrowser = win.getBrowser().selectedBrowser;

      let browsers = win.getBrowser().browsers;
      for each (let browser in browsers) {
        if (browser == selectedBrowser && win == top) {
          selected = tabActorList.length;
        }
        let actor = this._tabActors.get(browser);
        if (!actor) {
          actor = new BrowserTabActor(this.conn, browser, win.gBrowser);
          actor.parentID = this.actorID;
          this._tabActors.set(browser, actor);
        }
        actorPool.addActor(actor);
        tabActorList.push(actor);
      }
    }

    this._createExtraActors(DebuggerServer.globalActorFactories, actorPool);

    
    
    if (this._tabActorPool) {
      this.conn.removeActorPool(this._tabActorPool);
    }
    this._tabActorPool = actorPool;
    this.conn.addActorPool(this._tabActorPool);

    let response = {
      "from": "root",
      "selected": selected,
      "tabs": [actor.grip() for (actor of tabActorList)],
      "chromeDebugger": this._chromeDebugger.actorID
    };
    this._appendExtraActors(response);
    return response;
  },

  


  _createExtraActors: function BRA_createExtraActors(aFactories, aPool) {
    
    for (let name in aFactories) {
      let actor = this._extraActors[name];
      if (!actor) {
        actor = aFactories[name].bind(null, this.conn, this);
        actor.prototype = aFactories[name].prototype;
        actor.parentID = this.actorID;
        this._extraActors[name] = actor;
      }
      aPool.addActor(actor);
    }
  },

  


  _appendExtraActors: function BRA_appendExtraActors(aObject) {
    for (let name in this._extraActors) {
      let actor = this._extraActors[name];
      aObject[name] = actor.actorID;
    }
  },

  



  watchWindow: function BRA_watchWindow(aWindow) {
    this.getTabContainer(aWindow).addEventListener("TabClose",
                                                   this.onTabClosed,
                                                   false);
  },

  


  unwatchWindow: function BRA_unwatchWindow(aWindow) {
    this.getTabContainer(aWindow).removeEventListener("TabClose",
                                                      this.onTabClosed);
    this.exitTabActor(aWindow);
  },

  


  getTabContainer: function BRA_getTabContainer(aWindow) {
    return aWindow.getBrowser().tabContainer;
  },

  



  onTabClosed: function BRA_onTabClosed(aEvent) {
    this.exitTabActor(aEvent.target.linkedBrowser);
  },

  


  exitTabActor: function BRA_exitTabActor(aWindow) {
    let actor = this._tabActors.get(aWindow);
    if (actor) {
      actor.exit();
    }
  },

  

  







  addToParentPool: function BRA_addToParentPool(aActor) {
    this.conn.addActor(aActor);
  },

  





  removeFromParentPool: function BRA_removeFromParentPool(aActor) {
    this.conn.removeActor(aActor);
  },

  


  preNest: function BRA_preNest() {
    
    let e = windowMediator.getEnumerator(null);
    while (e.hasMoreElements()) {
      let win = e.getNext();
      let windowUtils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      windowUtils.suppressEventHandling(true);
      windowUtils.suspendTimeouts();
    }
  },

  


  postNest: function BRA_postNest(aNestData) {
    
    let e = windowMediator.getEnumerator(null);
    while (e.hasMoreElements()) {
      let win = e.getNext();
      let windowUtils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindowUtils);
      windowUtils.resumeTimeouts();
      windowUtils.suppressEventHandling(false);
    }
  },

  

  onWindowTitleChange: function BRA_onWindowTitleChange(aWindow, aTitle) { },
  onOpenWindow: function BRA_onOpenWindow(aWindow) { },
  onCloseWindow: function BRA_onCloseWindow(aWindow) {
    
    
    
    if (aWindow.getBrowser) {
      this.unwatchWindow(aWindow);
    }
  }
}




BrowserRootActor.prototype.requestTypes = {
  "listTabs": BrowserRootActor.prototype.onListTabs
};












function BrowserTabActor(aConnection, aBrowser, aTabBrowser)
{
  this.conn = aConnection;
  this._browser = aBrowser;
  this._tabbrowser = aTabBrowser;
  this._tabActorPool = null;
  
  this._extraActors = {};

  this._createExtraActors = BrowserRootActor.prototype._createExtraActors.bind(this);
  this._appendExtraActors = BrowserRootActor.prototype._appendExtraActors.bind(this);
  this._onWindowCreated = this.onWindowCreated.bind(this);
}




BrowserTabActor.prototype = {
  get browser() { return this._browser; },

  get exited() { return !this.browser; },
  get attached() { return !!this._attached },

  _tabPool: null,
  get tabActorPool() { return this._tabPool; },

  _contextPool: null,
  get contextActorPool() { return this._contextPool; },

  _pendingNavigation: null,

  







  addToParentPool: function BTA_addToParentPool(aActor) {
    this.conn.addActor(aActor);
  },

  





  removeFromParentPool: function BTA_removeFromParentPool(aActor) {
    this.conn.removeActor(aActor);
  },

  
  actorPrefix: "tab",

  




  get title() {
    let title = this.browser.contentTitle;
    
    
    
    if (!title && this._tabbrowser) {
      title = this._tabbrowser
                  ._getTabForContentWindow(this.contentWindow).label;
    }
    return title;
  },

  




  get url() {
    return this.browser.currentURI.spec;
  },

  




  get contentWindow() {
    return this.browser.contentWindow;
  },

  grip: function BTA_grip() {
    dbg_assert(!this.exited,
               "grip() shouldn't be called on exited browser actor.");
    dbg_assert(this.actorID,
               "tab should have an actorID.");

    let response = {
      actor: this.actorID,
      title: this.title,
      url: this.url,
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
  },

  


  exit: function BTA_exit() {
    if (this.exited) {
      return;
    }

    if (this.attached) {
      this._detach();
      this.conn.send({ from: this.actorID,
                       type: "tabDetached" });
    }

    this._browser = null;
    this._tabbrowser = null;
  },

  


  _attach: function BTA_attach() {
    if (this._attached) {
      return;
    }

    
    dbg_assert(!this._tabPool, "Shouldn't have a tab pool if we weren't attached.");
    this._tabPool = new ActorPool(this.conn);
    this.conn.addActorPool(this._tabPool);

    
    this._pushContext();

    
    this.browser.addEventListener("DOMWindowCreated", this._onWindowCreated, true);
    this.browser.addEventListener("pageshow", this._onWindowCreated, true);
    if (this._tabbrowser) {
      this._progressListener = new DebuggerProgressListener(this);
    }

    this._attached = true;
  },

  



  _pushContext: function BTA_pushContext() {
    dbg_assert(!this._contextPool, "Can't push multiple contexts");

    this._contextPool = new ActorPool(this.conn);
    this.conn.addActorPool(this._contextPool);

    this.threadActor = new ThreadActor(this, this.contentWindow.wrappedJSObject);
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
      return;
    }

    if (this._progressListener) {
      this._progressListener.destroy();
    }

    this.browser.removeEventListener("DOMWindowCreated", this._onWindowCreated, true);
    this.browser.removeEventListener("pageshow", this._onWindowCreated, true);

    this._popContext();

    
    this.conn.removeActorPool(this._tabPool);
    this._tabPool = null;
    if (this._tabActorPool) {
      this.conn.removeActorPool(this._tabActorPool, true);
      this._tabActorPool = null;
    }

    this._attached = false;
  },

  

  onAttach: function BTA_onAttach(aRequest) {
    if (this.exited) {
      return { type: "exited" };
    }

    this._attach();

    return { type: "tabAttached", threadActor: this.threadActor.actorID };
  },

  onDetach: function BTA_onDetach(aRequest) {
    if (!this.attached) {
      return { error: "wrongState" };
    }

    this._detach();

    return { type: "detached" };
  },

  


  preNest: function BTA_preNest() {
    if (!this.browser) {
      
      return;
    }
    let windowUtils = this.contentWindow
                          .QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);
    windowUtils.suppressEventHandling(true);
    windowUtils.suspendTimeouts();
  },

  


  postNest: function BTA_postNest(aNestData) {
    if (!this.browser) {
      
      return;
    }
    let windowUtils = this.contentWindow
                          .QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIDOMWindowUtils);
    windowUtils.resumeTimeouts();
    windowUtils.suppressEventHandling(false);
    if (this._pendingNavigation) {
      this._pendingNavigation.resume();
      this._pendingNavigation = null;
    }
  },

  



  onWindowCreated: function BTA_onWindowCreated(evt) {
    if (evt.target === this.browser.contentDocument) {
      
      
      if (evt.type == "pageshow" && !evt.persisted) {
        return;
      }
      if (this._attached) {
        this.threadActor.clearDebuggees();
        if (this.threadActor.dbg) {
          this.threadActor.dbg.enabled = true;
        }
      }
    }

    if (this._attached) {
      this.threadActor.global = evt.target.defaultView.wrappedJSObject;
      if (this.threadActor.attached) {
        this.threadActor.findGlobals();
      }
    }
  },

  








  hasNativeConsoleAPI: function BTA_hasNativeConsoleAPI(aWindow) {
    let isNative = false;
    try {
      let console = aWindow.wrappedJSObject.console;
      isNative = "__mozillaConsole__" in console;
    }
    catch (ex) { }
    return isNative;
  },
};




BrowserTabActor.prototype.requestTypes = {
  "attach": BrowserTabActor.prototype.onAttach,
  "detach": BrowserTabActor.prototype.onDetach
};










function DebuggerProgressListener(aBrowserTabActor) {
  this._tabActor = aBrowserTabActor;
  this._tabActor._tabbrowser.addProgressListener(this);
}

DebuggerProgressListener.prototype = {
  onStateChange:
  function DPL_onStateChange(aProgress, aRequest, aFlag, aStatus) {
    let isStart = aFlag & Ci.nsIWebProgressListener.STATE_START;
    let isStop = aFlag & Ci.nsIWebProgressListener.STATE_STOP;
    let isDocument = aFlag & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT;
    let isNetwork = aFlag & Ci.nsIWebProgressListener.STATE_IS_NETWORK;
    let isRequest = aFlag & Ci.nsIWebProgressListener.STATE_IS_REQUEST;
    let isWindow = aFlag & Ci.nsIWebProgressListener.STATE_IS_WINDOW;

    
    if (!isWindow || !isNetwork ||
        aProgress.DOMWindow != this._tabActor.contentWindow) {
      return;
    }

    if (isStart && aRequest instanceof Ci.nsIChannel) {
      
      

      
      if (this._tabActor.threadActor.state == "paused") {
        aRequest.suspend();
        this._tabActor.threadActor.onResume();
        this._tabActor.threadActor.dbg.enabled = false;
        this._tabActor._pendingNavigation = aRequest;
      }

      this._tabActor.conn.send({
        from: this._tabActor.actorID,
        type: "tabNavigated",
        url: aRequest.URI.spec,
        nativeConsoleAPI: true,
        state: "start",
      });
    } else if (isStop) {
      if (this._tabActor.threadActor.state == "running") {
        this._tabActor.threadActor.dbg.enabled = true;
      }

      let window = this._tabActor.contentWindow;
      this._tabActor.conn.send({
        from: this._tabActor.actorID,
        type: "tabNavigated",
        url: this._tabActor.url,
        title: this._tabActor.title,
        nativeConsoleAPI: this._tabActor.hasNativeConsoleAPI(window),
        state: "stop",
      });
    }
  },

  


  destroy: function DPL_destroy() {
    if (this._tabActor._tabbrowser.removeProgressListener) {
      try {
        this._tabActor._tabbrowser.removeProgressListener(this);
      } catch (ex) {
        
      }
    }
    this._tabActor._progressListener = null;
    this._tabActor = null;
  }
};
