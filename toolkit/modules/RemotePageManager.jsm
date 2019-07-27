



"use strict";

this.EXPORTED_SYMBOLS = ["RemotePages", "RemotePageManager", "PageListener"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function MessageListener() {
  this.listeners = new Map();
}

MessageListener.prototype = {
  keys: function() {
    return this.listeners.keys();
  },

  has: function(name) {
    return this.listeners.has(name);
  },

  callListeners: function(message) {
    let listeners = this.listeners.get(message.name);
    if (!listeners) {
      return;
    }

    for (let listener of listeners.values()) {
      try {
        listener(message);
      }
      catch (e) {
        Cu.reportError(e);
      }
    }
  },

  addMessageListener: function(name, callback) {
    if (!this.listeners.has(name))
      this.listeners.set(name, new Set([callback]));
    else
      this.listeners.get(name).add(callback);
  },

  removeMessageListener: function(name, callback) {
    if (!this.listeners.has(name))
      return;

    this.listeners.get(name).delete(callback);
  },
}








this.RemotePages = function(url) {
  this.url = url;
  this.messagePorts = new Set();
  this.listener = new MessageListener();
  this.destroyed = false;

  RemotePageManager.addRemotePageListener(url, this.portCreated.bind(this));
  this.portMessageReceived = this.portMessageReceived.bind(this);
}

RemotePages.prototype = {
  url: null,
  messagePorts: null,
  listener: null,
  destroyed: null,

  destroy: function() {
    RemotePageManager.removeRemotePageListener(this.url);

    for (let port of this.messagePorts.values()) {
      this.removeMessagePort(port);
    }

    this.messagePorts = null;
    this.listener = null;
    this.destroyed = true;
  },

  
  portCreated: function(port) {
    this.messagePorts.add(port);

    port.addMessageListener("RemotePage:Unload", this.portMessageReceived);

    for (let name of this.listener.keys()) {
      this.registerPortListener(port, name);
    }

    this.listener.callListeners({ target: port, name: "RemotePage:Init" });
  },

  
  portMessageReceived: function(message) {
    this.listener.callListeners(message);

    if (message.name == "RemotePage:Unload")
      this.removeMessagePort(message.target);
  },

  
  removeMessagePort: function(port) {
    for (let name of this.listener.keys()) {
      port.removeMessageListener(name, this.portMessageReceived);
    }

    port.removeMessageListener("RemotePage:Unload", this.portMessageReceived);
    this.messagePorts.delete(port);
  },

  registerPortListener: function(port, name) {
    port.addMessageListener(name, this.portMessageReceived);
  },

  
  sendAsyncMessage: function(name, data = null) {
    for (let port of this.messagePorts.values()) {
      port.sendAsyncMessage(name, data);
    }
  },

  addMessageListener: function(name, callback) {
    if (this.destroyed) {
      throw new Error("RemotePages has been destroyed");
    }

    if (!this.listener.has(name)) {
      for (let port of this.messagePorts.values()) {
        this.registerPortListener(port, name)
      }
    }

    this.listener.addMessageListener(name, callback);
  },

  removeMessageListener: function(name, callback) {
    if (this.destroyed) {
      throw new Error("RemotePages has been destroyed");
    }

    this.listener.removeMessageListener(name, callback);
  },
};



function publicMessagePort(port) {
  let properties = ["addMessageListener", "removeMessageListener",
                    "sendAsyncMessage", "destroy"];

  let clean = {};
  for (let property of properties) {
    clean[property] = port[property].bind(port);
  }

  if (port instanceof ChromeMessagePort) {
    Object.defineProperty(clean, "browser", {
      get: function() {
        return port.browser;
      }
    });
  }

  return clean;
}










function MessagePort(messageManager, portID) {
  this.messageManager = messageManager;
  this.portID = portID;
  this.destroyed = false;
  this.listener = new MessageListener();

  this.message = this.message.bind(this);
  this.messageManager.addMessageListener("RemotePage:Message", this.message);
}

MessagePort.prototype = {
  messageManager: null,
  portID: null,
  destroyed: null,
  listener: null,
  _browser: null,
  remotePort: null,

  
  
  swapMessageManager: function(messageManager) {
    this.messageManager.removeMessageListener("RemotePage:Message", this.message);

    this.messageManager = messageManager;

    this.messageManager.addMessageListener("RemotePage:Message", this.message);
  },

  







  addMessageListener: function(name, callback) {
    if (this.destroyed) {
      throw new Error("Message port has been destroyed");
    }

    this.listener.addMessageListener(name, callback);
  },

  


  removeMessageListener: function(name, callback) {
    if (this.destroyed) {
      throw new Error("Message port has been destroyed");
    }

    this.listener.removeMessageListener(name, callback);
  },

  
  sendAsyncMessage: function(name, data = null) {
    if (this.destroyed) {
      throw new Error("Message port has been destroyed");
    }

    this.messageManager.sendAsyncMessage("RemotePage:Message", {
      portID: this.portID,
      name: name,
      data: data,
    });
  },

  
  destroy: function() {
    try {
      
      this.messageManager.removeMessageListener("RemotePage:Message", this.message);
    }
    catch (e) { }
    this.messageManager = null;
    this.destroyed = true;
    this.portID = null;
    this.listener = null;
  },
};



function ChromeMessagePort(browser, portID) {
  MessagePort.call(this, browser.messageManager, portID);

  this._browser = browser;
  this._permanentKey = browser.permanentKey;

  Services.obs.addObserver(this, "message-manager-disconnect", false);
  this.publicPort = publicMessagePort(this);

  this.swapBrowsers = this.swapBrowsers.bind(this);
  this._browser.addEventListener("SwapDocShells", this.swapBrowsers, false);
}

ChromeMessagePort.prototype = Object.create(MessagePort.prototype);

Object.defineProperty(ChromeMessagePort.prototype, "browser", {
  get: function() {
    return this._browser;
  }
});



ChromeMessagePort.prototype.swapBrowsers = function({ detail: newBrowser }) {
  
  
  if (this._browser.permanentKey != this._permanentKey)
    return;

  this._browser.removeEventListener("SwapDocShells", this.swapBrowsers, false);

  this._browser = newBrowser;
  this.swapMessageManager(newBrowser.messageManager);

  this._browser.addEventListener("SwapDocShells", this.swapBrowsers, false);
}



ChromeMessagePort.prototype.observe = function(messageManager) {
  if (messageManager != this.messageManager)
    return;

  this.listener.callListeners({
    target: this.publicPort,
    name: "RemotePage:Unload",
    data: null,
  });
  this.destroy();
};



ChromeMessagePort.prototype.message = function({ data: messagedata }) {
  if (this.destroyed || (messagedata.portID != this.portID)) {
    return;
  }

  let message = {
    target: this.publicPort,
    name: messagedata.name,
    data: messagedata.data,
  };
  this.listener.callListeners(message);

  if (messagedata.name == "RemotePage:Unload")
    this.destroy();
};

ChromeMessagePort.prototype.destroy = function() {
  this._browser.removeEventListener("SwapDocShells", this.swapBrowsers, false);
  this._browser = null;
  Services.obs.removeObserver(this, "message-manager-disconnect");
  MessagePort.prototype.destroy.call(this);
};



function ChildMessagePort(contentFrame, window) {
  let portID = Services.appinfo.processID + ":" + ChildMessagePort.prototype.nextPortID++;
  MessagePort.call(this, contentFrame, portID);

  this.window = window;

  
  Cu.exportFunction(this.sendAsyncMessage.bind(this), window, {
    defineAs: "sendAsyncMessage",
  });
  Cu.exportFunction(this.addMessageListener.bind(this), window, {
    defineAs: "addMessageListener",
    allowCallbacks: true,
  });
  Cu.exportFunction(this.removeMessageListener.bind(this), window, {
    defineAs: "removeMessageListener",
    allowCallbacks: true,
  });

  
  let loadListener = () => {
    this.sendAsyncMessage("RemotePage:Load");
    window.removeEventListener("load", loadListener, false);
  };
  window.addEventListener("load", loadListener, false);

  
  window.addEventListener("unload", () => {
    try {
      this.sendAsyncMessage("RemotePage:Unload");
    }
    catch (e) {
      
      
    }
    this.destroy();
  }, false);

  
  this.messageManager.sendAsyncMessage("RemotePage:InitPort", {
    portID: portID,
    url: window.location.toString(),
  });
}

ChildMessagePort.prototype = Object.create(MessagePort.prototype);

ChildMessagePort.prototype.nextPortID = 0;



ChildMessagePort.prototype.message = function({ data: messagedata }) {
  if (this.destroyed || (messagedata.portID != this.portID)) {
    return;
  }

  let message = {
    name: messagedata.name,
    data: messagedata.data,
  };
  this.listener.callListeners(Cu.cloneInto(message, this.window));
};

ChildMessagePort.prototype.destroy = function() {
  this.window = null;
  MessagePort.prototype.destroy.call(this);
}



let RemotePageManagerInternal = {
  
  pages: new Map(),

  
  init: function() {
    Services.mm.addMessageListener("RemotePage:InitListener", this.initListener.bind(this));
    Services.mm.addMessageListener("RemotePage:InitPort", this.initPort.bind(this));
  },

  
  
  
  addRemotePageListener: function(url, callback) {
    if (Services.appinfo.processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT)
      throw new Error("RemotePageManager can only be used in the main process.");

    if (this.pages.has(url)) {
      throw new Error("Remote page already registered: " + url);
    }

    this.pages.set(url, callback);

    
    Services.mm.broadcastAsyncMessage("RemotePage:Register", { urls: [url] });
  },

  
  removeRemotePageListener: function(url) {
    if (Services.appinfo.processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT)
      throw new Error("RemotePageManager can only be used in the main process.");

    if (!this.pages.has(url)) {
      throw new Error("Remote page is not registered: " + url);
    }

    
    Services.mm.broadcastAsyncMessage("RemotePage:Unregister", { urls: [url] });
    this.pages.delete(url);
  },

  
  initListener: function({ target: browser }) {
    browser.messageManager.sendAsyncMessage("RemotePage:Register", { urls: [u for (u of this.pages.keys())] })
  },

  
  initPort: function({ target: browser, data: { url, portID } }) {
    let callback = this.pages.get(url);
    if (!callback) {
      Cu.reportError("Unexpected remote page load: " + url);
      return;
    }

    let port = new ChromeMessagePort(browser, portID);
    callback(port.publicPort);
  }
};

if (Services.appinfo.processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT)
  RemotePageManagerInternal.init();


this.RemotePageManager = {
  addRemotePageListener: RemotePageManagerInternal.addRemotePageListener.bind(RemotePageManagerInternal),
  removeRemotePageListener: RemotePageManagerInternal.removeRemotePageListener.bind(RemotePageManagerInternal),
};



function PageListener(contentFrame) {
  let registeredURLs = new Set();

  let observer = (window) => {
    
    if (window.top != contentFrame.content)
      return;

    let url = window.location.toString();
    if (!registeredURLs.has(url))
      return;

    
    let port = new ChildMessagePort(contentFrame, window);
  };
  Services.obs.addObserver(observer, "chrome-document-global-created", false);
  Services.obs.addObserver(observer, "content-document-global-created", false);

  
  contentFrame.addMessageListener("RemotePage:Register", ({ data }) => {
    for (let url of data.urls)
      registeredURLs.add(url);
  });

  
  contentFrame.addMessageListener("RemotePage:Unregister", ({ data }) => {
    for (let url of data.urls)
      registeredURLs.delete(url);
  });

  contentFrame.sendAsyncMessage("RemotePage:InitListener");
}
