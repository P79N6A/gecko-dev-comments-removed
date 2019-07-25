





"use strict";


(function _HUDServiceContent() {
let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

let tempScope = {};
Cu.import("resource://gre/modules/XPCOMUtils.jsm", tempScope);
Cu.import("resource://gre/modules/Services.jsm", tempScope);
Cu.import("resource://gre/modules/ConsoleAPIStorage.jsm", tempScope);
Cu.import("resource:///modules/WebConsoleUtils.jsm", tempScope);

let XPCOMUtils = tempScope.XPCOMUtils;
let Services = tempScope.Services;
let gConsoleStorage = tempScope.ConsoleAPIStorage;
let WebConsoleUtils = tempScope.WebConsoleUtils;
let l10n = WebConsoleUtils.l10n;
tempScope = null;

let _alive = true; 




let Manager = {
  get window() content,
  get console() this.window.console,
  sandbox: null,
  hudId: null,
  _sequence: 0,
  _messageListeners: ["WebConsole:Init", "WebConsole:EnableFeature",
                      "WebConsole:DisableFeature", "WebConsole:Destroy"],
  _messageHandlers: null,
  _enabledFeatures: null,

  


  get sequenceId() "HUDContent-" + (++this._sequence),

  


  init: function Manager_init()
  {
    this._enabledFeatures = [];
    this._messageHandlers = {};

    this._messageListeners.forEach(function(aName) {
      addMessageListener(aName, this);
    }, this);

    
    
    let xulWindow = this.window.QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIWebNavigation)
                    .QueryInterface(Ci.nsIDocShell)
                    .chromeEventHandler.ownerDocument.defaultView;

    xulWindow.addEventListener("unload", this._onXULWindowClose, false);

    let tabContainer = xulWindow.gBrowser.tabContainer;
    tabContainer.addEventListener("TabClose", this._onTabClose, false);

    
    
    
    
    
    Services.obs.addObserver(this, "private-browsing-change-granted", false);
    Services.obs.addObserver(this, "quit-application-granted", false);
  },

  



  receiveMessage: function Manager_receiveMessage(aMessage)
  {
    if (!_alive) {
      return;
    }

    if (!aMessage.json || (aMessage.name != "WebConsole:Init" &&
                           aMessage.json.hudId != this.hudId)) {
      Cu.reportError("Web Console content script: received message " +
                     aMessage.name + " from wrong hudId!");
      return;
    }

    switch (aMessage.name) {
      case "WebConsole:Init":
        this._onInit(aMessage.json);
        break;
      case "WebConsole:EnableFeature":
        this.enableFeature(aMessage.json.feature, aMessage.json);
        break;
      case "WebConsole:DisableFeature":
        this.disableFeature(aMessage.json.feature);
        break;
      case "WebConsole:Destroy":
        this.destroy();
        break;
      default: {
        let handler = this._messageHandlers[aMessage.name];
        handler && handler(aMessage.json);
        break;
      }
    }
  },

  






  observe: function Manager_observe(aSubject, aTopic, aData)
  {
    if (_alive && (aTopic == "quit-application-granted" ||
        (aTopic == "private-browsing-change-granted" &&
         (aData == "enter" || aData == "exit")))) {
      this.destroy();
    }
  },

  
























  _onInit: function Manager_onInit(aMessage)
  {
    this.hudId = aMessage.hudId;
    if (aMessage.features) {
      aMessage.features.forEach(function(aFeature) {
        this.enableFeature(aFeature, aMessage[aFeature]);
      }, this);
    }

    if (aMessage.cachedMessages) {
      this._sendCachedMessages(aMessage.cachedMessages);
    }
  },

  











  addMessageHandler: function Manager_addMessageHandler(aName, aCallback)
  {
    if (aName in this._messageHandlers) {
      Cu.reportError("Web Console content script: addMessageHandler() called for an existing message handler: " + aName);
      return;
    }

    this._messageHandlers[aName] = aCallback;
    addMessageListener(aName, this);
  },

  





  removeMessageHandler: function Manager_removeMessageHandler(aName)
  {
    if (!(aName in this._messageHandlers)) {
      return;
    }

    delete this._messageHandlers[aName];
    removeMessageListener(aName, this);
  },

  







  sendMessage: function Manager_sendMessage(aName, aMessage)
  {
    aMessage.hudId = this.hudId;
    if (!("id" in aMessage)) {
      aMessage.id = this.sequenceId;
    }

    sendAsyncMessage(aName, aMessage);
  },

  


















  enableFeature: function Manager_enableFeature(aFeature, aMessage)
  {
    if (this._enabledFeatures.indexOf(aFeature) != -1) {
      return;
    }

    switch (aFeature) {
      case "JSTerm":
        JSTerm.init(aMessage);
        break;
      case "ConsoleAPI":
        ConsoleAPIObserver.init(aMessage);
        break;
      case "PageError":
        ConsoleListener.init(aMessage);
        break;
      default:
        Cu.reportError("Web Console content: unknown feature " + aFeature);
        break;
    }

    this._enabledFeatures.push(aFeature);
  },

  







  disableFeature: function Manager_disableFeature(aFeature)
  {
    let index = this._enabledFeatures.indexOf(aFeature);
    if (index == -1) {
      return;
    }
    this._enabledFeatures.splice(index, 1);

    switch (aFeature) {
      case "JSTerm":
        JSTerm.destroy();
        break;
      case "ConsoleAPI":
        ConsoleAPIObserver.destroy();
        break;
      case "PageError":
        ConsoleListener.destroy();
        break;
      default:
        Cu.reportError("Web Console content: unknown feature " + aFeature);
        break;
    }
  },

  







  _sendCachedMessages: function Manager__sendCachedMessages(aMessageTypes)
  {
    let messages = [];

    while (aMessageTypes.length > 0) {
      switch (aMessageTypes.shift()) {
        case "ConsoleAPI":
          messages.push.apply(messages, ConsoleAPIObserver.getCachedMessages());
          break;
        case "PageError":
          messages.push.apply(messages, ConsoleListener.getCachedMessages());
          break;
      }
    }

    messages.sort(function(a, b) { return a.timeStamp - b.timeStamp; });

    this.sendMessage("WebConsole:CachedMessages", {messages: messages});
  },

  




  _onXULWindowClose: function Manager__onXULWindowClose()
  {
    if (_alive) {
      Manager.destroy();
    }
  },

  




  _onTabClose: function Manager__onTabClose(aEvent)
  {
    let tab = aEvent.target;
    if (_alive && tab.linkedBrowser.contentWindow === Manager.window) {
      Manager.destroy();
    }
  },

  


  destroy: function Manager_destroy()
  {
    Services.obs.removeObserver(this, "private-browsing-change-granted");
    Services.obs.removeObserver(this, "quit-application-granted");

    _alive = false;
    let xulWindow = this.window.QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIWebNavigation)
                    .QueryInterface(Ci.nsIDocShell)
                    .chromeEventHandler.ownerDocument.defaultView;

    xulWindow.removeEventListener("unload", this._onXULWindowClose, false);
    let tabContainer = xulWindow.gBrowser.tabContainer;
    tabContainer.removeEventListener("TabClose", this._onTabClose, false);

    this._messageListeners.forEach(function(aName) {
      removeMessageListener(aName, this);
    }, this);

    this._enabledFeatures.slice().forEach(this.disableFeature, this);

    this.hudId = null;
    this._messageHandlers = null;
    Manager = ConsoleAPIObserver = JSTerm = ConsoleListener = null;
    Cc = Ci = Cu = XPCOMUtils = Services = gConsoleStorage =
      WebConsoleUtils = l10n = null;
  },
};





let JSTerm = {
  



  _objectCache: null,

  


  init: function JST_init()
  {
    this._objectCache = {};

    Manager.addMessageHandler("JSTerm:GetEvalObject",
                              this.handleGetEvalObject.bind(this));
    Manager.addMessageHandler("JSTerm:ClearObjectCache",
                              this.handleClearObjectCache.bind(this));
  },

  
















  handleGetEvalObject: function JST_handleGetEvalObject(aRequest)
  {
    if (aRequest.cacheId in this._objectCache &&
        aRequest.objectId in this._objectCache[aRequest.cacheId]) {
      let object = this._objectCache[aRequest.cacheId][aRequest.objectId];
      let resultCacheId = aRequest.resultCacheId || aRequest.cacheId;
      let message = {
        id: aRequest.id,
        cacheId: aRequest.cacheId,
        objectId: aRequest.objectId,
        object: this.prepareObjectForRemote(object, resultCacheId),
        childrenCacheId: resultCacheId,
      };
      Manager.sendMessage("JSTerm:EvalObject", message);
    }
    else {
      Cu.reportError("JSTerm:GetEvalObject request " + aRequest.id +
                     ": stale object.");
    }
  },

  







  handleClearObjectCache: function JST_handleClearObjectCache(aRequest)
  {
    if (aRequest.cacheId in this._objectCache) {
      delete this._objectCache[aRequest.cacheId];
    }
  },

  














  prepareObjectForRemote:
  function JST_prepareObjectForRemote(aObject, aCacheId)
  {
    
    let propCache = this._objectCache[aCacheId] || {};
    let result = WebConsoleUtils.namesAndValuesOf(aObject, propCache);
    if (!(aCacheId in this._objectCache) && Object.keys(propCache).length > 0) {
      this._objectCache[aCacheId] = propCache;
    }

    return result;
  },

  


  destroy: function JST_destroy()
  {
    Manager.removeMessageHandler("JSTerm:GetEvalObject");
    Manager.removeMessageHandler("JSTerm:ClearObjectCache");

    delete this._objectCache;
  },
};





let ConsoleAPIObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  


  init: function CAO_init()
  {
    
    
    Services.obs.addObserver(this, "console-api-log-event", false);

    Manager.addMessageHandler("ConsoleAPI:ClearCache",
                              this.handleClearCache.bind(this));
  },

  








  observe: function CAO_observe(aMessage, aTopic)
  {
    if (!_alive || !aMessage || aTopic != "console-api-log-event") {
      return;
    }

    let apiMessage = aMessage.wrappedJSObject;

    let msgWindow =
      WebConsoleUtils.getWindowByOuterId(apiMessage.ID, Manager.window);
    if (!msgWindow || msgWindow.top != Manager.window) {
      
      return;
    }

    let messageToChrome = {};
    this._prepareApiMessageForRemote(apiMessage, messageToChrome);
    Manager.sendMessage("WebConsole:ConsoleAPI", messageToChrome);
  },

  























  _prepareApiMessageForRemote:
  function CAO__prepareApiMessageForRemote(aOriginalMessage, aRemoteMessage)
  {
    aRemoteMessage.apiMessage =
      WebConsoleUtils.cloneObject(aOriginalMessage, true,
        function(aKey, aValue, aObject) {
          
          if (aKey == "wrappedJSObject" || aObject === aOriginalMessage &&
              aKey == "arguments") {
            return false;
          }
          return true;
        });

    aRemoteMessage.timeStamp = aOriginalMessage.timeStamp;

    switch (aOriginalMessage.level) {
      case "trace":
      case "time":
      case "timeEnd":
      case "group":
      case "groupCollapsed":
        aRemoteMessage.apiMessage.arguments =
          WebConsoleUtils.cloneObject(aOriginalMessage.arguments, true);
        break;

      case "log":
      case "info":
      case "warn":
      case "error":
      case "debug":
      case "groupEnd":
        aRemoteMessage.argumentsToString =
          Array.map(aOriginalMessage.arguments || [],
                    this._formatObject.bind(this));
        break;

      case "dir": {
        aRemoteMessage.objectsCacheId = Manager.sequenceId;
        aRemoteMessage.argumentsToString = [];
        let mapFunction = function(aItem) {
          aRemoteMessage.argumentsToString.push(this._formatObject(aItem));
          if (WebConsoleUtils.isObjectInspectable(aItem)) {
            return JSTerm.prepareObjectForRemote(aItem,
                                                 aRemoteMessage.objectsCacheId);
          }
          return aItem;
        }.bind(this);

        aRemoteMessage.apiMessage.arguments =
          Array.map(aOriginalMessage.arguments || [], mapFunction);
        break;
      }
      default:
        Cu.reportError("Unknown Console API log level: " +
                       aOriginalMessage.level);
        break;
    }
  },

  








  _formatObject: function CAO__formatObject(aObject)
  {
    return typeof aObject == "string" ?
           aObject : WebConsoleUtils.formatResult(aObject);
  },

  







  getCachedMessages: function CAO_getCachedMessages()
  {
    let innerWindowId = WebConsoleUtils.getInnerWindowId(Manager.window);
    let messages = gConsoleStorage.getEvents(innerWindowId);

    let result = messages.map(function(aMessage) {
      let remoteMessage = { _type: "ConsoleAPI" };
      this._prepareApiMessageForRemote(aMessage.wrappedJSObject, remoteMessage);
      return remoteMessage;
    }, this);

    return result;
  },

  


  handleClearCache: function CAO_handleClearCache()
  {
    let windowId = WebConsoleUtils.getInnerWindowId(Manager.window);
    gConsoleStorage.clearEvents(windowId);
  },

  


  destroy: function CAO_destroy()
  {
    Manager.removeMessageHandler("ConsoleAPI:ClearCache");
    Services.obs.removeObserver(this, "console-api-log-event");
  },
};





let ConsoleListener = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIConsoleListener]),

  


  init: function CL_init()
  {
    Services.console.registerListener(this);
  },

  







  observe: function CL_observe(aScriptError)
  {
    if (!_alive || !(aScriptError instanceof Ci.nsIScriptError) ||
        !aScriptError.outerWindowID) {
      return;
    }

    switch (aScriptError.category) {
      
      case "XPConnect JavaScript":
      case "component javascript":
      case "chrome javascript":
      case "chrome registration":
      case "XBL":
      case "XBL Prototype Handler":
      case "XBL Content Sink":
      case "xbl javascript":
        return;
    }

    let errorWindow =
      WebConsoleUtils.getWindowByOuterId(aScriptError.outerWindowID,
                                         Manager.window);
    if (!errorWindow || errorWindow.top != Manager.window) {
      return;
    }

    Manager.sendMessage("WebConsole:PageError", { pageError: aScriptError });
  },

  







  getCachedMessages: function CL_getCachedMessages()
  {
    let innerWindowId = WebConsoleUtils.getInnerWindowId(Manager.window);
    let result = [];
    let errors = {};
    Services.console.getMessageArray(errors, {});

    (errors.value || []).forEach(function(aError) {
      if (!(aError instanceof Ci.nsIScriptError) ||
          aError.innerWindowID != innerWindowId) {
        return;
      }

      let remoteMessage = WebConsoleUtils.cloneObject(aError);
      remoteMessage._type = "PageError";
      result.push(remoteMessage);
    });

    return result;
  },

  


  destroy: function CL_destroy()
  {
    Services.console.unregisterListener(this);
  },
};

Manager.init();
})();
