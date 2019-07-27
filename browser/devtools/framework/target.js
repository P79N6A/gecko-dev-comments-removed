



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {Promise: promise} = require("resource://gre/modules/Promise.jsm");
const EventEmitter = require("devtools/toolkit/event-emitter");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DebuggerClient",
  "resource://gre/modules/devtools/dbg-client.jsm");

const targets = new WeakMap();
const promiseTargets = new WeakMap();




exports.TargetFactory = {
  






  forTab: function TF_forTab(tab) {
    let target = targets.get(tab);
    if (target == null) {
      target = new TabTarget(tab);
      targets.set(tab, target);
    }
    return target;
  },

  












  forRemoteTab: function TF_forRemoteTab(options) {
    let targetPromise = promiseTargets.get(options);
    if (targetPromise == null) {
      let target = new TabTarget(options);
      targetPromise = target.makeRemote().then(() => target);
      promiseTargets.set(options, targetPromise);
    }
    return targetPromise;
  },

  






  isKnownTab: function TF_isKnownTab(tab) {
    return targets.has(tab);
  },

  





  forWindow: function TF_forWindow(window) {
    let target = targets.get(window);
    if (target == null) {
      target = new WindowTarget(window);
      targets.set(window, target);
    }
    return target;
  },

  



  allTargets: function TF_allTargets() {
    let windows = [];
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Ci.nsIWindowMediator);
    let en = wm.getXULWindowEnumerator(null);
    while (en.hasMoreElements()) {
      windows.push(en.getNext());
    }

    return windows.map(function(window) {
      return TargetFactory.forWindow(window);
    });
  },
};








function getVersion() {
  
  return 20;
}






function supports(feature) {
  
  return false;
};



































function Target() {
  throw new Error("Use TargetFactory.newXXX or Target.getXXX to create a Target in place of 'new Target()'");
}

Object.defineProperty(Target.prototype, "version", {
  get: getVersion,
  enumerable: true
});






function TabTarget(tab) {
  EventEmitter.decorate(this);
  this.destroy = this.destroy.bind(this);
  this._handleThreadState = this._handleThreadState.bind(this);
  this.on("thread-resumed", this._handleThreadState);
  this.on("thread-paused", this._handleThreadState);
  
  
  if (tab && !["client", "form", "chrome"].every(tab.hasOwnProperty, tab)) {
    this._tab = tab;
    this._setupListeners();
  } else {
    this._form = tab.form;
    this._client = tab.client;
    this._chrome = tab.chrome;
  }
}

TabTarget.prototype = {
  _webProgressListener: null,

  supports: supports,
  get version() { return getVersion(); },

  get tab() {
    return this._tab;
  },

  get form() {
    return this._form;
  },

  get root() {
    return this._root;
  },

  get client() {
    return this._client;
  },

  get chrome() {
    return this._chrome;
  },

  get window() {
    
    
    
    
    if (Services.appinfo.processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT) {
      Cu.reportError("The .window getter on devtools' |target| object isn't e10s friendly!\n"
                     + Error().stack);
    }
    
    
    if (this._tab && this._tab.linkedBrowser) {
      return this._tab.linkedBrowser.contentWindow;
    }
    return null;
  },

  get name() {
    return this._tab && this._tab.linkedBrowser.contentDocument ?
           this._tab.linkedBrowser.contentDocument.title :
           this._form.title;
  },

  get url() {
    return this._tab ? this._tab.linkedBrowser.currentURI.spec :
                       this._form.url;
  },

  get isRemote() {
    return !this.isLocalTab;
  },

  get isAddon() {
    return !!(this._form && this._form.addonActor);
  },

  get isLocalTab() {
    return !!this._tab;
  },

  get isThreadPaused() {
    return !!this._isThreadPaused;
  },

  




  makeRemote: function TabTarget_makeRemote() {
    if (this._remote) {
      return this._remote.promise;
    }

    this._remote = promise.defer();

    if (this.isLocalTab) {
      
      
      if (!DebuggerServer.initialized) {
        DebuggerServer.init();
        DebuggerServer.addBrowserActors();
      }

      this._client = new DebuggerClient(DebuggerServer.connectPipe());
      
      this._chrome = false;
    }

    this._setupRemoteListeners();

    let attachTab = () => {
      this._client.attachTab(this._form.actor, (aResponse, aTabClient) => {
        if (!aTabClient) {
          this._remote.reject("Unable to attach to the tab");
          return;
        }
        this.activeTab = aTabClient;
        this.threadActor = aResponse.threadActor;
        this._remote.resolve(null);
      });
    };

    if (this.isLocalTab) {
      this._client.connect((aType, aTraits) => {
        this._client.listTabs(aResponse => {
          this._root = aResponse;

          if (this.window) {
            let windowUtils = this.window
              .QueryInterface(Ci.nsIInterfaceRequestor)
              .getInterface(Ci.nsIDOMWindowUtils);
            let outerWindow = windowUtils.outerWindowID;
            aResponse.tabs.some((tab) => {
              if (tab.outerWindowID === outerWindow) {
                this._form = tab;
                return true;
              }
              return false;
            });
          }

          if (!this._form) {
            this._form = aResponse.tabs[aResponse.selected];
          }
          attachTab();
        });
      });
    } else if (!this.chrome) {
      
      
      attachTab();
    } else {
      
      this._remote.resolve(null);
    }

    return this._remote.promise;
  },

  


  _setupListeners: function TabTarget__setupListeners() {
    this._webProgressListener = new TabWebProgressListener(this);
    this.tab.linkedBrowser.addProgressListener(this._webProgressListener);
    this.tab.addEventListener("TabClose", this);
    this.tab.parentNode.addEventListener("TabSelect", this);
    this.tab.ownerDocument.defaultView.addEventListener("unload", this);
  },

  


  _teardownListeners: function TabTarget__teardownListeners() {
    if (this._webProgressListener) {
      this._webProgressListener.destroy();
    }

    this._tab.ownerDocument.defaultView.removeEventListener("unload", this);
    this._tab.removeEventListener("TabClose", this);
    this._tab.parentNode.removeEventListener("TabSelect", this);
  },

  


  _setupRemoteListeners: function TabTarget__setupRemoteListeners() {
    this.client.addListener("closed", this.destroy);

    this._onTabDetached = (aType, aPacket) => {
      
      if (aPacket.from == this._form.actor) {
        this.destroy();
      }
    };
    this.client.addListener("tabDetached", this._onTabDetached);

    this._onTabNavigated = (aType, aPacket) => {
      let event = Object.create(null);
      event.url = aPacket.url;
      event.title = aPacket.title;
      event.nativeConsoleAPI = aPacket.nativeConsoleAPI;
      event.isFrameSwitching = aPacket.isFrameSwitching;
      
      
      if (aPacket.state == "start") {
        event._navPayload = this._navRequest;
        this.emit("will-navigate", event);
        this._navRequest = null;
      } else {
        event._navPayload = this._navWindow;
        this.emit("navigate", event);
        this._navWindow = null;
      }
    };
    this.client.addListener("tabNavigated", this._onTabNavigated);

    this._onFrameUpdate = (aType, aPacket) => {
      this.emit("frame-update", aPacket);
    };
    this.client.addListener("frameUpdate", this._onFrameUpdate);
  },

  


  _teardownRemoteListeners: function TabTarget__teardownRemoteListeners() {
    this.client.removeListener("closed", this.destroy);
    this.client.removeListener("tabNavigated", this._onTabNavigated);
    this.client.removeListener("tabDetached", this._onTabDetached);
    this.client.removeListener("frameUpdate", this._onFrameUpdate);
  },

  


  handleEvent: function (event) {
    switch (event.type) {
      case "TabClose":
      case "unload":
        this.destroy();
        break;
      case "TabSelect":
        if (this.tab.selected) {
          this.emit("visible", event);
        } else {
          this.emit("hidden", event);
        }
        break;
    }
  },

  


  _handleThreadState: function(event) {
    switch (event) {
      case "thread-resumed":
        this._isThreadPaused = false;
        break;
      case "thread-paused":
        this._isThreadPaused = true;
        break;
    }
  },

  


  destroy: function() {
    
    
    if (this._destroyer) {
      return this._destroyer.promise;
    }

    this._destroyer = promise.defer();

    
    this.emit("close");

    
    
    this.off("thread-resumed", this._handleThreadState);
    this.off("thread-paused", this._handleThreadState);

    if (this._tab) {
      this._teardownListeners();
    }

    let cleanupAndResolve = () => {
      this._cleanup();
      this._destroyer.resolve(null);
    };
    
    
    if (this._tab && !this._client) {
      cleanupAndResolve();
    } else if (this._client) {
      
      
      this._teardownRemoteListeners();

      if (this.isLocalTab) {
        
        
        this._client.close(cleanupAndResolve);
      } else {
        
        
        if (this.activeTab) {
          this.activeTab.detach(cleanupAndResolve);
        } else {
          cleanupAndResolve();
        }
      }
    }

    return this._destroyer.promise;
  },

  


  _cleanup: function TabTarget__cleanup() {
    if (this._tab) {
      targets.delete(this._tab);
    } else {
      promiseTargets.delete(this._form);
    }
    this.activeTab = null;
    this._client = null;
    this._tab = null;
    this._form = null;
    this._remote = null;
  },

  toString: function() {
    return 'TabTarget:' + (this._tab ? this._tab : (this._form && this._form.actor));
  },
};








function TabWebProgressListener(aTarget) {
  this.target = aTarget;
}

TabWebProgressListener.prototype = {
  target: null,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener, Ci.nsISupportsWeakReference]),

  onStateChange: function TWPL_onStateChange(progress, request, flag, status) {
    let isStart = flag & Ci.nsIWebProgressListener.STATE_START;
    let isDocument = flag & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT;
    let isNetwork = flag & Ci.nsIWebProgressListener.STATE_IS_NETWORK;
    let isRequest = flag & Ci.nsIWebProgressListener.STATE_IS_REQUEST;

    
    if (!isStart || !isDocument || !isRequest || !isNetwork) {
      return;
    }

    
    if (progress.isTopLevel) {
      
      
      if (this.target._client) {
        this.target._navRequest = request;
      } else {
        this.target.emit("will-navigate", request);
      }
    }
  },

  onProgressChange: function() {},
  onSecurityChange: function() {},
  onStatusChange: function() {},

  onLocationChange: function TWPL_onLocationChange(webProgress, request, URI, flags) {
    if (this.target &&
        !(flags & Ci.nsIWebProgressListener.LOCATION_CHANGE_SAME_DOCUMENT)) {
      let window = webProgress.DOMWindow;
      
      
      if (this.target._client) {
        this.target._navWindow = window;
      } else {
        this.target.emit("navigate", window);
      }
    }
  },

  


  destroy: function TWPL_destroy() {
    if (this.target.tab) {
      try {
        this.target.tab.linkedBrowser.removeProgressListener(this);
      } catch (ex) {
        
      }
    }
    this.target._webProgressListener = null;
    this.target._navRequest = null;
    this.target._navWindow = null;
    this.target = null;
  }
};






function WindowTarget(window) {
  EventEmitter.decorate(this);
  this._window = window;
  this._setupListeners();
}

WindowTarget.prototype = {
  supports: supports,
  get version() { return getVersion(); },

  get window() {
    return this._window;
  },

  get name() {
    return this._window.document.title;
  },

  get url() {
    return this._window.document.location.href;
  },

  get isRemote() {
    return false;
  },

  get isLocalTab() {
    return false;
  },

  get isThreadPaused() {
    return !!this._isThreadPaused;
  },

  


  _setupListeners: function() {
    this._handleThreadState = this._handleThreadState.bind(this);
    this.on("thread-paused", this._handleThreadState);
    this.on("thread-resumed", this._handleThreadState);
  },

  _handleThreadState: function(event) {
    switch (event) {
      case "thread-resumed":
        this._isThreadPaused = false;
        break;
      case "thread-paused":
        this._isThreadPaused = true;
        break;
    }
  },

  


  destroy: function() {
    if (!this._destroyed) {
      this._destroyed = true;

      this.off("thread-paused", this._handleThreadState);
      this.off("thread-resumed", this._handleThreadState);
      this.emit("close");

      targets.delete(this._window);
      this._window = null;
    }

    return promise.resolve(null);
  },

  toString: function() {
    return 'WindowTarget:' + this.window;
  },
};
