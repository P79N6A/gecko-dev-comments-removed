



this.EXPORTED_SYMBOLS = ["RemoteAddonsParent"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import('resource://gre/modules/Services.jsm');

XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
                                  "resource://gre/modules/BrowserUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Prefetcher",
                                  "resource://gre/modules/Prefetcher.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CompatWarning",
                                  "resource://gre/modules/CompatWarning.jsm");



function setDefault(dict, key, default_)
{
  if (key in dict) {
    return dict[key];
  }
  dict[key] = default_;
  return default_;
}







let NotificationTracker = {
  
  
  
  
  
  
  _paths: {},

  init: function() {
    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.addMessageListener("Addons:GetNotifications", this);
  },

  add: function(path) {
    let tracked = this._paths;
    for (let component of path) {
      tracked = setDefault(tracked, component, {});
    }
    let count = tracked._count || 0;
    count++;
    tracked._count = count;

    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.broadcastAsyncMessage("Addons:ChangeNotification", {path: path, count: count});
  },

  remove: function(path) {
    let tracked = this._paths;
    for (let component of path) {
      tracked = setDefault(tracked, component, {});
    }
    tracked._count--;

    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.broadcastAsyncMessage("Addons:ChangeNotification", {path: path, count: tracked._count});
  },

  receiveMessage: function(msg) {
    if (msg.name == "Addons:GetNotifications") {
      return this._paths;
    }
  }
};
NotificationTracker.init();





function Interposition(name, base)
{
  this.name = name;
  if (base) {
    this.methods = Object.create(base.methods);
    this.getters = Object.create(base.getters);
    this.setters = Object.create(base.setters);
  } else {
    this.methods = Object.create(null);
    this.getters = Object.create(null);
    this.setters = Object.create(null);
  }
}




let ContentPolicyParent = {
  init: function() {
    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.addMessageListener("Addons:ContentPolicy:Run", this);

    this._policies = new Map();
  },

  addContentPolicy: function(addon, name, cid) {
    this._policies.set(name, cid);
    NotificationTracker.add(["content-policy", addon]);
  },

  removeContentPolicy: function(addon, name) {
    this._policies.delete(name);
    NotificationTracker.remove(["content-policy", addon]);
  },

  receiveMessage: function (aMessage) {
    switch (aMessage.name) {
      case "Addons:ContentPolicy:Run":
        return this.shouldLoad(aMessage.data, aMessage.objects);
        break;
    }
  },

  shouldLoad: function(aData, aObjects) {
    for (let policyCID of this._policies.values()) {
      let policy;
      try {
        policy = Cc[policyCID].getService(Ci.nsIContentPolicy);
      } catch (e) {
        
        continue;
      }
      try {
        let contentLocation = BrowserUtils.makeURI(aData.contentLocation);
        let requestOrigin = aData.requestOrigin ? BrowserUtils.makeURI(aData.requestOrigin) : null;

        let result = Prefetcher.withPrefetching(aData.prefetched, aObjects, () => {
          return policy.shouldLoad(aData.contentType,
                                   contentLocation,
                                   requestOrigin,
                                   aObjects.node,
                                   aData.mimeTypeGuess,
                                   null,
                                   aData.requestPrincipal);
        });
        if (result != Ci.nsIContentPolicy.ACCEPT && result != 0)
          return result;
      } catch (e) {
        Cu.reportError(e);
      }
    }

    return Ci.nsIContentPolicy.ACCEPT;
  },
};
ContentPolicyParent.init();



let CategoryManagerInterposition = new Interposition("CategoryManagerInterposition");

CategoryManagerInterposition.methods.addCategoryEntry =
  function(addon, target, category, entry, value, persist, replace) {
    if (category == "content-policy") {
      CompatWarning.warn("content-policy should be added from the child process only.",
                         addon, CompatWarning.warnings.nsIContentPolicy);
      ContentPolicyParent.addContentPolicy(addon, entry, value);
    }

    target.addCategoryEntry(category, entry, value, persist, replace);
  };

CategoryManagerInterposition.methods.deleteCategoryEntry =
  function(addon, target, category, entry, persist) {
    if (category == "content-policy") {
      CompatWarning.warn("content-policy should be removed from the child process only.",
                         addon, CompatWarning.warnings.nsIContentPolicy);
      ContentPolicyParent.removeContentPolicy(addon, entry);
    }

    target.deleteCategoryEntry(category, entry, persist);
  };




let AboutProtocolParent = {
  init: function() {
    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.addMessageListener("Addons:AboutProtocol:GetURIFlags", this);
    ppmm.addMessageListener("Addons:AboutProtocol:OpenChannel", this);
    this._protocols = [];
  },

  registerFactory: function(addon, class_, className, contractID, factory) {
    this._protocols.push({contractID: contractID, factory: factory});
    NotificationTracker.add(["about-protocol", contractID, addon]);
  },

  unregisterFactory: function(addon, class_, factory) {
    for (let i = 0; i < this._protocols.length; i++) {
      if (this._protocols[i].factory == factory) {
        NotificationTracker.remove(["about-protocol", this._protocols[i].contractID, addon]);
        this._protocols.splice(i, 1);
        break;
      }
    }
  },

  receiveMessage: function (msg) {
    switch (msg.name) {
      case "Addons:AboutProtocol:GetURIFlags":
        return this.getURIFlags(msg);
      case "Addons:AboutProtocol:OpenChannel":
        return this.openChannel(msg);
        break;
    }
  },

  getURIFlags: function(msg) {
    let uri = BrowserUtils.makeURI(msg.data.uri);
    let contractID = msg.data.contractID;
    let module = Cc[contractID].getService(Ci.nsIAboutModule);
    try {
      return module.getURIFlags(uri);
    } catch (e) {
      Cu.reportError(e);
    }
  },

  
  
  openChannel: function(msg) {
    function wrapGetInterface(cpow) {
      return {
        getInterface: function(intf) { return cpow.getInterface(intf); }
      };
    }

    let uri = BrowserUtils.makeURI(msg.data.uri);
    let contractID = msg.data.contractID;
    let loadingPrincipal = msg.data.loadingPrincipal;
    let securityFlags = msg.data.securityFlags;
    let contentPolicyType = msg.data.contentPolicyType;
    try {
      let channel = NetUtil.newChannel2(uri,
                                        null,
                                        null,
                                        null,  
                                        loadingPrincipal,
                                        null,  
                                        securityFlags,
                                        contentPolicyType);

      
      
      
      
      channel.notificationCallbacks = wrapGetInterface(msg.objects.notificationCallbacks);
      if (msg.objects.loadGroupNotificationCallbacks) {
        channel.loadGroup = {notificationCallbacks: msg.objects.loadGroupNotificationCallbacks};
      } else {
        channel.loadGroup = null;
      }
      let stream = channel.open();
      let data = NetUtil.readInputStreamToString(stream, stream.available(), {});
      return {
        data: data,
        contentType: channel.contentType
      };
    } catch (e) {
      Cu.reportError(e);
    }
  },
};
AboutProtocolParent.init();

let ComponentRegistrarInterposition = new Interposition("ComponentRegistrarInterposition");

ComponentRegistrarInterposition.methods.registerFactory =
  function(addon, target, class_, className, contractID, factory) {
    if (contractID && contractID.startsWith("@mozilla.org/network/protocol/about;1?")) {
      CompatWarning.warn("nsIAboutModule should be registered in the content process" +
                         " as well as the chrome process. (If you do that already, ignore" +
                         " this warning.)",
                         addon, CompatWarning.warnings.nsIAboutModule);
      AboutProtocolParent.registerFactory(addon, class_, className, contractID, factory);
    }

    target.registerFactory(class_, className, contractID, factory);
  };

ComponentRegistrarInterposition.methods.unregisterFactory =
  function(addon, target, class_, factory) {
    AboutProtocolParent.unregisterFactory(addon, class_, factory);
    target.unregisterFactory(class_, factory);
  };










let ObserverParent = {
  init: function() {
    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
               .getService(Ci.nsIMessageBroadcaster);
    ppmm.addMessageListener("Addons:Observer:Run", this);
  },

  addObserver: function(addon, observer, topic, ownsWeak) {
    Services.obs.addObserver(observer, "e10s-" + topic, ownsWeak);
    NotificationTracker.add(["observer", topic, addon]);
  },

  removeObserver: function(addon, observer, topic) {
    Services.obs.removeObserver(observer, "e10s-" + topic);
    NotificationTracker.remove(["observer", topic, addon]);
  },

  receiveMessage: function(msg) {
    switch (msg.name) {
      case "Addons:Observer:Run":
        this.notify(msg.objects.subject, msg.objects.topic, msg.objects.data);
        break;
    }
  },

  notify: function(subject, topic, data) {
    let e = Services.obs.enumerateObservers("e10s-" + topic);
    while (e.hasMoreElements()) {
      let obs = e.getNext().QueryInterface(Ci.nsIObserver);
      try {
        obs.observe(subject, topic, data);
      } catch (e) {
        Cu.reportError(e);
      }
    }
  }
};
ObserverParent.init();


let TOPIC_WHITELIST = [
  "content-document-global-created",
  "document-element-inserted",
  "dom-window-destroyed",
  "inner-window-destroyed",
  "outer-window-destroyed",
];



let ObserverInterposition = new Interposition("ObserverInterposition");

ObserverInterposition.methods.addObserver =
  function(addon, target, observer, topic, ownsWeak) {
    if (TOPIC_WHITELIST.indexOf(topic) >= 0) {
      ObserverParent.addObserver(addon, observer, topic);
    }

    CompatWarning.warn(`${topic} observer should be added from the child process only.`,
                       addon, CompatWarning.warnings.observers);
    target.addObserver(observer, topic, ownsWeak);
  };

ObserverInterposition.methods.removeObserver =
  function(addon, target, observer, topic) {
    if (TOPIC_WHITELIST.indexOf(topic) >= 0) {
      ObserverParent.removeObserver(addon, observer, topic);
    }

    target.removeObserver(observer, topic);
  };



let EventTargetParent = {
  init: function() {
    
    
    this._listeners = new WeakMap();

    let mm = Cc["@mozilla.org/globalmessagemanager;1"].
      getService(Ci.nsIMessageListenerManager);
    mm.addMessageListener("Addons:Event:Run", this);
  },

  
  
  
  
  
  
  redirectEventTarget: function(target) {
    if (Cu.isCrossProcessWrapper(target)) {
      return null;
    }

    if (target instanceof Ci.nsIDOMChromeWindow) {
      return target;
    }

    if (target instanceof Ci.nsIDOMXULElement) {
      if (target.localName == "browser") {
        return target;
      } else if (target.localName == "tab") {
        return target.linkedBrowser;
      }

      
      
      let window = target.ownerDocument.defaultView;
      if (window && target.contains(window.gBrowser)) {
        return window;
      }
    }

    return null;
  },

  
  
  
  getTargets: function(browser) {
    let window = browser.ownerDocument.defaultView;
    return [browser, window];
  },

  addEventListener: function(addon, target, type, listener, useCapture, wantsUntrusted) {
    let newTarget = this.redirectEventTarget(target);
    if (!newTarget) {
      return;
    }

    useCapture = useCapture || false;
    wantsUntrusted = wantsUntrusted || false;

    NotificationTracker.add(["event", type, useCapture, addon]);

    let listeners = this._listeners.get(newTarget);
    if (!listeners) {
      listeners = {};
      this._listeners.set(newTarget, listeners);
    }
    let forType = setDefault(listeners, type, []);

    
    for (let i = 0; i < forType.length; i++) {
      if (forType[i].listener === listener &&
          forType[i].target === target &&
          forType[i].useCapture === useCapture &&
          forType[i].wantsUntrusted === wantsUntrusted) {
        return;
      }
    }

    forType.push({listener: listener,
                  target: target,
                  wantsUntrusted: wantsUntrusted,
                  useCapture: useCapture});
  },

  removeEventListener: function(addon, target, type, listener, useCapture) {
    let newTarget = this.redirectEventTarget(target);
    if (!newTarget) {
      return;
    }

    useCapture = useCapture || false;

    let listeners = this._listeners.get(newTarget);
    if (!listeners) {
      return;
    }
    let forType = setDefault(listeners, type, []);

    for (let i = 0; i < forType.length; i++) {
      if (forType[i].listener === listener &&
          forType[i].target === target &&
          forType[i].useCapture === useCapture) {
        forType.splice(i, 1);
        NotificationTracker.remove(["event", type, useCapture, addon]);
        break;
      }
    }
  },

  receiveMessage: function(msg) {
    switch (msg.name) {
      case "Addons:Event:Run":
        this.dispatch(msg.target, msg.data.type, msg.data.capturing,
                      msg.data.isTrusted, msg.data.prefetched, msg.objects);
        break;
    }
  },

  dispatch: function(browser, type, capturing, isTrusted, prefetched, cpows) {
    let event = cpows.event;
    let eventTarget = cpows.eventTarget;
    let targets = this.getTargets(browser);
    for (let target of targets) {
      let listeners = this._listeners.get(target);
      if (!listeners) {
        continue;
      }
      let forType = setDefault(listeners, type, []);

      
      let handlers = [];
      for (let {listener, target, wantsUntrusted, useCapture} of forType) {
        if ((wantsUntrusted || isTrusted) && useCapture == capturing) {
          handlers.push([listener, target]);
        }
      }

      for (let [handler, target] of handlers) {
        let EventProxy = {
          get: function(knownProps, name) {
            if (knownProps.hasOwnProperty(name))
              return knownProps[name];
            return event[name];
          }
        }
        let proxyEvent = new Proxy({
          currentTarget: target,
          target: eventTarget,
          type: type,
          QueryInterface: function(iid) {
            if (iid.equals(Ci.nsISupports) ||
                iid.equals(Ci.nsIDOMEventTarget))
              return proxyEvent;
            
            
            event.QueryInterface(iid);
            return proxyEvent;
          }
        }, EventProxy);

        try {
          Prefetcher.withPrefetching(prefetched, cpows, () => {
            if ("handleEvent" in handler) {
              handler.handleEvent(proxyEvent);
            } else {
              handler.call(eventTarget, proxyEvent);
            }
          });
        } catch (e) {
          Cu.reportError(e);
        }
      }
    }
  }
};
EventTargetParent.init();





let filteringListeners = new WeakMap();
function makeFilteringListener(eventType, listener)
{
  if (filteringListeners.has(listener)) {
    return filteringListeners.get(listener);
  }

  
  
  let eventTypes = ["mousedown", "mouseup", "click"];
  if (eventTypes.indexOf(eventType) == -1) {
    return listener;
  }

  function filter(event) {
    let target = event.originalTarget;
    if (target instanceof Ci.nsIDOMXULElement &&
        target.localName == "browser" &&
        target.isRemoteBrowser) {
      return;
    }

    if ("handleEvent" in listener) {
      listener.handleEvent(event);
    } else {
      listener.call(event.target, event);
    }
  }
  filteringListeners.set(listener, filter);
  return filter;
}



let EventTargetInterposition = new Interposition("EventTargetInterposition");

EventTargetInterposition.methods.addEventListener =
  function(addon, target, type, listener, useCapture, wantsUntrusted) {
    CompatWarning.warn("Registering an event listener on content DOM nodes" +
                        " needs to happen in the content process.",
                       addon, CompatWarning.warnings.DOM_events);
    EventTargetParent.addEventListener(addon, target, type, listener, useCapture, wantsUntrusted);
    target.addEventListener(type, makeFilteringListener(type, listener), useCapture, wantsUntrusted);
  };

EventTargetInterposition.methods.removeEventListener =
  function(addon, target, type, listener, useCapture) {
    EventTargetParent.removeEventListener(addon, target, type, listener, useCapture);
    target.removeEventListener(type, makeFilteringListener(type, listener), useCapture);
  };





let ContentDocShellTreeItemInterposition = new Interposition("ContentDocShellTreeItemInterposition");

ContentDocShellTreeItemInterposition.getters.rootTreeItem =
  function(addon, target) {
    
    let chromeGlobal = target.rootTreeItem
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIContentFrameMessageManager);

    
    let browser = RemoteAddonsParent.globalToBrowser.get(chromeGlobal);
    if (!browser) {
      
      
      
      return null;
    }

    let chromeWin = browser.ownerDocument.defaultView;

    
    return chromeWin.QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation)
      .QueryInterface(Ci.nsIDocShellTreeItem);
  };

function chromeGlobalForContentWindow(window)
{
    return window
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation)
      .QueryInterface(Ci.nsIDocShellTreeItem)
      .rootTreeItem
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIContentFrameMessageManager);
}





let SandboxParent = {
  componentsMap: new WeakMap(),

  makeContentSandbox: function(addon, chromeGlobal, principals, ...rest) {
    CompatWarning.warn("This sandbox should be created from the child process.",
                       addon, CompatWarning.warnings.sandboxes);
    if (rest.length) {
      
      
      
      
      
      
      
      let options = rest[0];
      let optionsCopy = new chromeGlobal.Object();
      for (let prop in options) {
        optionsCopy[prop] = options[prop];
      }
      rest[0] = optionsCopy;
    }

    
    let cu = chromeGlobal.Components.utils;
    let sandbox = cu.Sandbox(principals, ...rest);

    
    
    
    chromeGlobal.addSandbox(sandbox);

    
    
    
    this.componentsMap.set(sandbox, cu);
    return sandbox;
  },

  evalInSandbox: function(code, sandbox, ...rest) {
    let cu = this.componentsMap.get(sandbox);
    return cu.evalInSandbox(code, sandbox, ...rest);
  }
};




let ComponentsUtilsInterposition = new Interposition("ComponentsUtilsInterposition");

ComponentsUtilsInterposition.methods.Sandbox =
  function(addon, target, principals, ...rest) {
    
    
    if (principals &&
        typeof(principals) == "object" &&
        Cu.isCrossProcessWrapper(principals) &&
        principals instanceof Ci.nsIDOMWindow) {
      let chromeGlobal = chromeGlobalForContentWindow(principals);
      return SandboxParent.makeContentSandbox(addon, chromeGlobal, principals, ...rest);
    } else if (principals &&
               typeof(principals) == "object" &&
               "every" in principals &&
               principals.length &&
               principals.every(e => e instanceof Ci.nsIDOMWindow && Cu.isCrossProcessWrapper(e))) {
      let chromeGlobal = chromeGlobalForContentWindow(principals[0]);

      
      
      let array = new chromeGlobal.Array();
      for (let i = 0; i < principals.length; i++) {
        array[i] = principals[i];
      }
      return SandboxParent.makeContentSandbox(addon, chromeGlobal, array, ...rest);
    } else {
      return Components.utils.Sandbox(principals, ...rest);
    }
  };

ComponentsUtilsInterposition.methods.evalInSandbox =
  function(addon, target, code, sandbox, ...rest) {
    if (sandbox && Cu.isCrossProcessWrapper(sandbox)) {
      return SandboxParent.evalInSandbox(code, sandbox, ...rest);
    } else {
      return Components.utils.evalInSandbox(code, sandbox, ...rest);
    }
  };





let ContentDocumentInterposition = new Interposition("ContentDocumentInterposition");

ContentDocumentInterposition.methods.importNode =
  function(addon, target, node, deep) {
    if (!Cu.isCrossProcessWrapper(node)) {
      
      
      
      
      Cu.reportError("Calling contentDocument.importNode on a XUL node is not allowed.");
      return node;
    }

    return target.importNode(node, deep);
  };



let RemoteBrowserElementInterposition = new Interposition("RemoteBrowserElementInterposition",
                                                          EventTargetInterposition);

RemoteBrowserElementInterposition.getters.docShell = function(addon, target) {
  CompatWarning.warn("Direct access to content docshell will no longer work in the chrome process.",
                     addon, CompatWarning.warnings.content);
  let remoteChromeGlobal = RemoteAddonsParent.browserToGlobal.get(target);
  if (!remoteChromeGlobal) {
    
    return null;
  }
  return remoteChromeGlobal.docShell;
};




function makeDummyContentWindow(browser) {
  let dummyContentWindow = {
    set location(url) {
      browser.loadURI(url, null, null);
    },
    document: {
      readyState: "loading",
      location: { href: "about:blank" }
    },
    frames: [],
  };
  dummyContentWindow.top = dummyContentWindow;
  dummyContentWindow.document.defaultView = dummyContentWindow;
  browser._contentWindow = dummyContentWindow;
  return dummyContentWindow;
}

RemoteBrowserElementInterposition.getters.contentWindow = function(addon, target) {
  CompatWarning.warn("Direct access to content objects will no longer work in the chrome process.",
                     addon, CompatWarning.warnings.content);

  
  
  
  if (!target.contentWindowAsCPOW) {
    CompatWarning.warn("CPOW to the content window does not exist yet, dummy content window is created.");
    return makeDummyContentWindow(target);
  }
  return target.contentWindowAsCPOW;
};

function getContentDocument(addon, browser)
{
  if (!browser.contentWindowAsCPOW) {
    return makeDummyContentWindow(browser).document;
  }

  let doc = Prefetcher.lookupInCache(addon, browser.contentWindowAsCPOW, "document");
  if (doc) {
    return doc;
  }

  return browser.contentDocumentAsCPOW;
}

RemoteBrowserElementInterposition.getters.contentDocument = function(addon, target) {
  CompatWarning.warn("Direct access to content objects will no longer work in the chrome process.",
                     addon, CompatWarning.warnings.content);

  return getContentDocument(addon, target);
};

let TabBrowserElementInterposition = new Interposition("TabBrowserElementInterposition",
                                                       EventTargetInterposition);

TabBrowserElementInterposition.getters.contentWindow = function(addon, target) {
  CompatWarning.warn("Direct access to content objects will no longer work in the chrome process.",
                     addon, CompatWarning.warnings.content);

  if (!target.selectedBrowser.contentWindowAsCPOW) {
    return makeDummyContentWindow(target.selectedBrowser);
  }
  return target.selectedBrowser.contentWindowAsCPOW;
};

TabBrowserElementInterposition.getters.contentDocument = function(addon, target) {
  CompatWarning.warn("Direct access to content objects will no longer work in the chrome process.",
                     addon, CompatWarning.warnings.content);

  let browser = target.selectedBrowser;
  return getContentDocument(addon, browser);
};

let ChromeWindowInterposition = new Interposition("ChromeWindowInterposition",
                                                  EventTargetInterposition);



ChromeWindowInterposition.getters.content =
ChromeWindowInterposition.getters._content = function(addon, target) {
  CompatWarning.warn("Direct access to content objects will no longer work in the chrome process.",
                     addon, CompatWarning.warnings.content);

  let browser = target.gBrowser.selectedBrowser;
  if (!browser.contentWindowAsCPOW) {
    return makeDummyContentWindow(browser);
  }
  return browser.contentWindowAsCPOW;
};

let RemoteAddonsParent = {
  init: function() {
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    mm.addMessageListener("Addons:RegisterGlobal", this);

    this.globalToBrowser = new WeakMap();
    this.browserToGlobal = new WeakMap();
  },

  getInterfaceInterpositions: function() {
    let result = {};

    function register(intf, interp) {
      result[intf.number] = interp;
    }

    register(Ci.nsICategoryManager, CategoryManagerInterposition);
    register(Ci.nsIComponentRegistrar, ComponentRegistrarInterposition);
    register(Ci.nsIObserverService, ObserverInterposition);
    register(Ci.nsIXPCComponents_Utils, ComponentsUtilsInterposition);

    return result;
  },

  getTaggedInterpositions: function() {
    let result = {};

    function register(tag, interp) {
      result[tag] = interp;
    }

    register("EventTarget", EventTargetInterposition);
    register("ContentDocShellTreeItem", ContentDocShellTreeItemInterposition);
    register("ContentDocument", ContentDocumentInterposition);
    register("RemoteBrowserElement", RemoteBrowserElementInterposition);
    register("TabBrowserElement", TabBrowserElementInterposition);
    register("ChromeWindow", ChromeWindowInterposition);

    return result;
  },

  receiveMessage: function(msg) {
    switch (msg.name) {
    case "Addons:RegisterGlobal":
      this.browserToGlobal.set(msg.target, msg.objects.global);
      this.globalToBrowser.set(msg.objects.global, msg.target);
      break;
    }
  }
};
