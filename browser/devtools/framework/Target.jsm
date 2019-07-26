



"use strict";

this.EXPORTED_SYMBOLS = [ "TargetFactory" ];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");


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

  










  forRemote: function TF_forRemote(form, client, chrome) {
    let target = targets.get(form);
    if (target == null) {
      target = new RemoteTarget(form, client, chrome);
      targets.set(form, target);
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
  this._tab = tab;
  this._setupListeners();
}

TabTarget.prototype = {
  _webProgressListener: null,

  supports: supports,
  get version() { return getVersion(); },

  get tab() {
    return this._tab;
  },

  get window() {
    return this._tab.linkedBrowser.contentWindow;
  },

  get name() {
    return this._tab.linkedBrowser.contentDocument.title;
  },

  get url() {
    return this._tab.linkedBrowser.contentDocument.location.href;
  },

  get isRemote() {
    return false;
  },

  get isLocalTab() {
    return true;
  },

  get isThreadPaused() {
    return !!this._isThreadPaused;
  },

  


  _setupListeners: function TabTarget__setupListeners() {
    this._webProgressListener = new TabWebProgressListener(this);
    this.tab.linkedBrowser.addProgressListener(this._webProgressListener);
    this.tab.addEventListener("TabClose", this);
    this.tab.parentNode.addEventListener("TabSelect", this);
    this.tab.ownerDocument.defaultView.addEventListener("unload", this);
    this._handleThreadState = this._handleThreadState.bind(this);
    this.on("thread-resumed", this._handleThreadState);
    this.on("thread-paused", this._handleThreadState);
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
    if (!this._destroyed) {
      this._destroyed = true;

      this.tab.linkedBrowser.removeProgressListener(this._webProgressListener)
      this._webProgressListener.target = null;
      this._webProgressListener = null;
      this.tab.ownerDocument.defaultView.removeEventListener("unload", this);
      this.tab.removeEventListener("TabClose", this);
      this.tab.parentNode.removeEventListener("TabSelect", this);
      this.off("thread-resumed", this._handleThreadState);
      this.off("thread-paused", this._handleThreadState);
      this.emit("close");

      targets.delete(this._tab);
      this._tab = null;
    }

    return Promise.resolve(null);
  },

  toString: function() {
    return 'TabTarget:' + this.tab;
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

  onLocationChange: function TwPL_onLocationChange(webProgress, request, URI, flags) {
    if (this.target &&
        !(flags & Ci.nsIWebProgressListener.LOCATION_CHANGE_SAME_DOCUMENT)) {
      let window = webProgress.DOMWindow;
      this.target.emit("navigate", window);
    }
  },
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




function RemoteTarget(form, client, chrome) {
  EventEmitter.decorate(this);
  this._client = client;
  this._form = form;
  this._chrome = chrome;
  this._setupListeners();
}

RemoteTarget.prototype = {
  supports: supports,
  get version() getVersion(),

  get isRemote() true,

  get chrome() this._chrome,

  get name() this._form.title,

  get url() this._form.url,

  get client() this._client,

  get form() this._form,

  get isLocalTab() false,

  get isThreadPaused() !!this._isThreadPaused,

  


  _setupListeners: function() {
    this.destroy = this.destroy.bind(this);
    this.client.addListener("tabDetached", this.destroy);

    this._onTabNavigated = function onRemoteTabNavigated(aType, aPacket) {
      if (aPacket.state == "start") {
        this.emit("will-navigate", aPacket);
      } else {
        this.emit("navigate", aPacket);
      }
    }.bind(this);
    this.client.addListener("tabNavigated", this._onTabNavigated);

    this._handleThreadState = this._handleThreadState.bind(this);
    this.on("thread-resumed", this._handleThreadState);
    this.on("thread-paused", this._handleThreadState);
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

  


  destroy: function RT_destroy() {
    
    
    if (this._destroyer) {
      return this._destroyer.promise;
    }

    this._destroyer = Promise.defer();

    this.client.removeListener("tabNavigated", this._onTabNavigated);
    this.client.removeListener("tabDetached", this.destroy);

    this._client.close(function onClosed() {
      this._client = null;
      this.off("thread-resumed", this._handleThreadState);
      this.off("thread-paused", this._handleThreadState);
      this.emit("close");

      this._destroyer.resolve(null);
    }.bind(this));

    return this._destroyer.promise;
  },

  toString: function() {
    return 'RemoteTarget:' + this.form.actor;
  },
};
