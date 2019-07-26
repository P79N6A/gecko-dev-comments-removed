



"use strict";

this.EXPORTED_SYMBOLS = [ "TargetFactory" ];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerClient",
  "resource://gre/modules/devtools/dbg-client.jsm");

const targets = new WeakMap();




this.TargetFactory = {
  







  forTab: function TF_forTab(tab) {
    let target = targets.get(tab);
    if (target == null) {
      target = new TabTarget(tab);
      targets.set(tab, target);
    }
    return target;
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
    let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
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

  get client() {
    return this._client;
  },

  get chrome() {
    return this._chrome;
  },

  get window() {
    
    
    if (this._tab && this._tab.linkedBrowser) {
      return this._tab.linkedBrowser.contentWindow;
    }
  },

  get name() {
    return this._tab ? this._tab.linkedBrowser.contentDocument.title :
                       this._form.title;
  },

  get url() {
    return this._tab ? this._tab.linkedBrowser.contentDocument.location.href :
                       this._form.url;
  },

  get isRemote() {
    return !this.isLocalTab;
  },

  get isLocalTab() {
    return !!this._tab;
  },

  get isThreadPaused() {
    return !!this._isThreadPaused;
  },

  








  makeRemote: function TabTarget_makeRemote(aOptions) {
    if (this._remote) {
      return this._remote.promise;
    }

    this._remote = Promise.defer();

    if (aOptions) {
      this._form = aOptions.form;
      this._client = aOptions.client;
      this._chrome = aOptions.chrome;
    } else {
      
      
      if (!DebuggerServer.initialized) {
        DebuggerServer.init();
        DebuggerServer.addBrowserActors();
      }

      this._client = new DebuggerClient(DebuggerServer.connectPipe());
      
      this._chrome = false;
    }

    this._setupRemoteListeners();

    if (aOptions) {
      
      
      this._remote.resolve(null);
    } else {
      this._client.connect(function(aType, aTraits) {
        this._client.listTabs(function(aResponse) {
          this._form = aResponse.tabs[aResponse.selected];
          this._remote.resolve(null);
        }.bind(this));
      }.bind(this));
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

  


  _setupRemoteListeners: function TabTarget__setupRemoteListeners() {
    
    if (this._webProgressListener) {
      this._webProgressListener.destroy();
    }

    this.client.addListener("tabDetached", this.destroy);

    this._onTabNavigated = function onRemoteTabNavigated(aType, aPacket) {
      if (aPacket.state == "start") {
        this.emit("will-navigate", aPacket);
      } else {
        this.emit("navigate", aPacket);
      }
    }.bind(this);
    this.client.addListener("tabNavigated", this._onTabNavigated);
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

    this._destroyer = Promise.defer();

    
    this.emit("close");

    
    
    this.off("thread-resumed", this._handleThreadState);
    this.off("thread-paused", this._handleThreadState);

    if (this._tab) {
      this._tab.ownerDocument.defaultView.removeEventListener("unload", this);
      this._tab.removeEventListener("TabClose", this);
      this._tab.parentNode.removeEventListener("TabSelect", this);
    }

    
    
    
    if (this._tab && !this._client) {
      if (this._webProgressListener) {
        this._webProgressListener.destroy();
      }

      targets.delete(this._tab);
      this._tab = null;
      this._client = null;
      this._form = null;
      this._remote = null;

      this._destroyer.resolve(null);
    } else if (this._client) {
      
      
      this.client.removeListener("tabNavigated", this._onTabNavigated);
      this.client.removeListener("tabDetached", this.destroy);

      this._client.close(function onClosed() {
        let key = this._tab ? this._tab : this._form;
        targets.delete(key);
        this._client = null;
        this._tab = null;
        this._form = null;
        this._remote = null;

        this._destroyer.resolve(null);
      }.bind(this));
    }

    return this._destroyer.promise;
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

    
    if (this.target && this.target.window == progress.DOMWindow) {
      this.target.emit("will-navigate", request);
    }
  },

  onProgressChange: function() {},
  onSecurityChange: function() {},
  onStatusChange: function() {},

  onLocationChange: function TWPL_onLocationChange(webProgress, request, URI, flags) {
    if (this.target &&
        !(flags & Ci.nsIWebProgressListener.LOCATION_CHANGE_SAME_DOCUMENT)) {
      let window = webProgress.DOMWindow;
      this.target.emit("navigate", window);
    }
  },

  


  destroy: function TWPL_destroy() {
    if (this.target.tab) {
      this.target.tab.linkedBrowser.removeProgressListener(this);
    }
    this.target._webProgressListener = null;
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

    return Promise.resolve(null);
  },

  toString: function() {
    return 'WindowTarget:' + this.window;
  },
};
