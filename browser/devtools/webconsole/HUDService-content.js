





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
Cu.import("resource:///modules/NetworkHelper.jsm", tempScope);
Cu.import("resource://gre/modules/NetUtil.jsm", tempScope);

let XPCOMUtils = tempScope.XPCOMUtils;
let Services = tempScope.Services;
let gConsoleStorage = tempScope.ConsoleAPIStorage;
let WebConsoleUtils = tempScope.WebConsoleUtils;
let l10n = WebConsoleUtils.l10n;
let JSPropertyProvider = tempScope.JSPropertyProvider;
let NetworkHelper = tempScope.NetworkHelper;
let NetUtil = tempScope.NetUtil;
tempScope = null;

let activityDistributor = Cc["@mozilla.org/network/http-activity-distributor;1"].getService(Ci.nsIHttpActivityDistributor);

let _alive = true; 




let Manager = {
  get window() content,
  hudId: null,
  _sequence: 0,
  _messageListeners: ["WebConsole:Init", "WebConsole:EnableFeature",
                      "WebConsole:DisableFeature", "WebConsole:SetPreferences",
                      "WebConsole:GetPreferences", "WebConsole:Destroy"],
  _messageHandlers: null,
  _enabledFeatures: null,
  _prefs: { },

  


  get sequenceId() "HUDContent-" + (++this._sequence),

  


  init: function Manager_init()
  {
    this._enabledFeatures = [];
    this._messageHandlers = {};

    this._messageListeners.forEach(function(aName) {
      addMessageListener(aName, this);
    }, this);

    
    
    let xulWindow = this._xulWindow();
    xulWindow.addEventListener("unload", this._onXULWindowClose, false);

    let tabContainer = xulWindow.gBrowser.tabContainer;
    tabContainer.addEventListener("TabClose", this._onTabClose, false);

    
    
    
    
    
    Services.obs.addObserver(this, "private-browsing-change-granted", false);
    Services.obs.addObserver(this, "quit-application-granted", false);
  },

  



  receiveMessage: function Manager_receiveMessage(aMessage)
  {
    if (!_alive || !aMessage.json) {
      return;
    }

    if (aMessage.name == "WebConsole:Init" && !this.hudId) {
      this._onInit(aMessage.json);
      return;
    }
    if (aMessage.json.hudId != this.hudId) {
      return;
    }

    switch (aMessage.name) {
      case "WebConsole:EnableFeature":
        this.enableFeature(aMessage.json.feature, aMessage.json);
        break;
      case "WebConsole:DisableFeature":
        this.disableFeature(aMessage.json.feature);
        break;
      case "WebConsole:GetPreferences":
        this.handleGetPreferences(aMessage.json);
        break;
      case "WebConsole:SetPreferences":
        this.handleSetPreferences(aMessage.json);
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

    if (aMessage.preferences) {
      this.handleSetPreferences({ preferences: aMessage.preferences });
    }

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
      case "NetworkMonitor":
        NetworkMonitor.init(aMessage);
        break;
      case "LocationChange":
        ConsoleProgressListener.startMonitor(ConsoleProgressListener
                                             .MONITOR_LOCATION_CHANGE);
        ConsoleProgressListener.sendLocation(this.window.location.href,
                                             this.window.document.title);
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
      case "NetworkMonitor":
        NetworkMonitor.destroy();
        break;
      case "LocationChange":
        ConsoleProgressListener.stopMonitor(ConsoleProgressListener
                                            .MONITOR_LOCATION_CHANGE);
        break;
      default:
        Cu.reportError("Web Console content: unknown feature " + aFeature);
        break;
    }
  },

  











  handleGetPreferences: function Manager_handleGetPreferences(aMessage)
  {
    let prefs = {};
    aMessage.preferences.forEach(function(aName) {
      prefs[aName] = this.getPreference(aName);
    }, this);

    this.sendMessage("WebConsole:Preferences", {preferences: prefs});
  },

  









  handleSetPreferences: function Manager_handleSetPreferences(aMessage)
  {
    for (let key in aMessage.preferences) {
      this.setPreference(key, aMessage.preferences[key]);
    }
  },

  








  getPreference: function Manager_getPreference(aName)
  {
    return aName in this._prefs ? this._prefs[aName] : null;
  },

  







  setPreference: function Manager_setPreference(aName, aValue)
  {
    this._prefs[aName] = aValue;
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

  





  _xulWindow: function Manager__xulWindow()
  {
    return this.window.QueryInterface(Ci.nsIInterfaceRequestor)
           .getInterface(Ci.nsIWebNavigation).QueryInterface(Ci.nsIDocShell)
           .chromeEventHandler.ownerDocument.defaultView;
  },

  


  destroy: function Manager_destroy()
  {
    Services.obs.removeObserver(this, "private-browsing-change-granted");
    Services.obs.removeObserver(this, "quit-application-granted");

    _alive = false;
    let xulWindow = this._xulWindow();
    xulWindow.removeEventListener("unload", this._onXULWindowClose, false);
    let tabContainer = xulWindow.gBrowser.tabContainer;
    tabContainer.removeEventListener("TabClose", this._onTabClose, false);

    this._messageListeners.forEach(function(aName) {
      removeMessageListener(aName, this);
    }, this);

    this._enabledFeatures.slice().forEach(this.disableFeature, this);

    this.hudId = null;
    this._messageHandlers = null;

    Manager = ConsoleAPIObserver = JSTerm = ConsoleListener = NetworkMonitor =
      NetworkResponseListener = ConsoleProgressListener = null;

    XPCOMUtils = gConsoleStorage = WebConsoleUtils = l10n = JSPropertyProvider =
      null;
  },
};














function JSTermHelper(aJSTerm)
{
  







  aJSTerm.sandbox.$ = function JSTH_$(aSelector)
  {
    return aJSTerm.window.document.querySelector(aSelector);
  };

  







  aJSTerm.sandbox.$$ = function JSTH_$$(aSelector)
  {
    return aJSTerm.window.document.querySelectorAll(aSelector);
  };

  








  aJSTerm.sandbox.$x = function JSTH_$x(aXPath, aContext)
  {
    let nodes = [];
    let doc = aJSTerm.window.document;
    let aContext = aContext || doc;

    try {
      let results = doc.evaluate(aXPath, aContext, null,
                                 Ci.nsIDOMXPathResult.ANY_TYPE, null);
      let node;
      while (node = results.iterateNext()) {
        nodes.push(node);
      }
    }
    catch (ex) {
      aJSTerm.console.error(ex.message);
    }

    return nodes;
  };

  









  Object.defineProperty(aJSTerm.sandbox, "$0", {
    get: function() {
      try {
        return Manager._xulWindow().InspectorUI.selection;
      }
      catch (ex) {
        aJSTerm.console.error(ex.message);
      }
    },
    enumerable: true,
    configurable: false
  });

  


  aJSTerm.sandbox.clear = function JSTH_clear()
  {
    aJSTerm.helperEvaluated = true;
    Manager.sendMessage("JSTerm:ClearOutput", {});
  };

  






  aJSTerm.sandbox.keys = function JSTH_keys(aObject)
  {
    return Object.keys(WebConsoleUtils.unwrap(aObject));
  };

  






  aJSTerm.sandbox.values = function JSTH_values(aObject)
  {
    let arrValues = [];
    let obj = WebConsoleUtils.unwrap(aObject);

    try {
      for (let prop in obj) {
        arrValues.push(obj[prop]);
      }
    }
    catch (ex) {
      aJSTerm.console.error(ex.message);
    }
    return arrValues;
  };

  


  aJSTerm.sandbox.help = function JSTH_help()
  {
    aJSTerm.helperEvaluated = true;
    aJSTerm.window.open(
        "https://developer.mozilla.org/docs/Tools/Web_Console/Helpers",
        "help", "");
  };

  





  aJSTerm.sandbox.inspect = function JSTH_inspect(aObject)
  {
    if (!WebConsoleUtils.isObjectInspectable(aObject)) {
      return aObject;
    }

    aJSTerm.helperEvaluated = true;

    let message = {
      input: aJSTerm._evalInput,
      objectCacheId: Manager.sequenceId,
    };

    message.resultObject =
      aJSTerm.prepareObjectForRemote(WebConsoleUtils.unwrap(aObject),
                                     message.objectCacheId);

    Manager.sendMessage("JSTerm:InspectObject", message);
  };

  






  aJSTerm.sandbox.pprint = function JSTH_pprint(aObject)
  {
    aJSTerm.helperEvaluated = true;
    if (aObject === null || aObject === undefined || aObject === true ||
        aObject === false) {
      aJSTerm.console.error(l10n.getStr("helperFuncUnsupportedTypeError"));
      return;
    }
    else if (typeof aObject == "function") {
      aJSTerm.helperRawOutput = true;
      return aObject + "\n";
    }

    aJSTerm.helperRawOutput = true;

    let output = [];
    let pairs = WebConsoleUtils.namesAndValuesOf(WebConsoleUtils.unwrap(aObject));
    pairs.forEach(function(aPair) {
      output.push(aPair.name + ": " + aPair.value);
    });

    return "  " + output.join("\n  ");
  };

  






  aJSTerm.sandbox.print = function JSTH_print(aString)
  {
    aJSTerm.helperEvaluated = true;
    aJSTerm.helperRawOutput = true;
    return String(aString);
  };
}





let JSTerm = {
  get window() Manager.window,
  get console() this.window.console,

  


  sandbox: null,

  _sandboxLocation: null,
  _messageHandlers: {},

  



  _objectCache: null,

  












  init: function JST_init(aMessage)
  {
    this._objectCache = {};
    this._messageHandlers = {
      "JSTerm:EvalRequest": this.handleEvalRequest,
      "JSTerm:GetEvalObject": this.handleGetEvalObject,
      "JSTerm:Autocomplete": this.handleAutocomplete,
      "JSTerm:ClearObjectCache": this.handleClearObjectCache,
    };

    for (let name in this._messageHandlers) {
      let handler = this._messageHandlers[name].bind(this);
      Manager.addMessageHandler(name, handler);
    }

    if (aMessage && aMessage.notifyNonNativeConsoleAPI) {
      let consoleObject = WebConsoleUtils.unwrap(this.window).console;
      if (!("__mozillaConsole__" in consoleObject)) {
        Manager.sendMessage("JSTerm:NonNativeConsoleAPI", {});
      }
    }
  },

  
































  handleEvalRequest: function JST_handleEvalRequest(aRequest)
  {
    let id = aRequest.id;
    let input = aRequest.str;
    let result, error = null;
    let timestamp;

    this.helperEvaluated = false;
    this.helperRawOutput = false;
    this._evalInput = input;
    try {
      timestamp = Date.now();
      result = this.evalInSandbox(input);
    }
    catch (ex) {
      error = ex;
    }
    delete this._evalInput;

    let inspectable = !error && WebConsoleUtils.isObjectInspectable(result);
    let resultString = undefined;
    if (!error) {
      resultString = this.helperRawOutput ? result :
                     WebConsoleUtils.formatResult(result);
    }

    let message = {
      id: id,
      input: input,
      resultString: resultString,
      timestamp: timestamp,
      error: error,
      errorMessage: error ? String(error) : null,
      inspectable: inspectable,
      helperResult: this.helperEvaluated,
      helperRawOutput: this.helperRawOutput,
    };

    if (inspectable) {
      message.childrenCacheId = aRequest.resultCacheId;
      message.resultObject =
        this.prepareObjectForRemote(result, message.childrenCacheId);
    }

    Manager.sendMessage("JSTerm:EvalResult", message);
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

  














  prepareObjectForRemote: function JST_prepareObjectForRemote(aObject, aCacheId)
  {
    
    let propCache = this._objectCache[aCacheId] || {};
    let result = WebConsoleUtils.namesAndValuesOf(aObject, propCache);
    if (!(aCacheId in this._objectCache) && Object.keys(propCache).length > 0) {
      this._objectCache[aCacheId] = propCache;
    }

    return result;
  },

  















  handleAutocomplete: function JST_handleAutocomplete(aRequest)
  {
    let result = JSPropertyProvider(this.window, aRequest.input) || {};
    let message = {
      id: aRequest.id,
      input: aRequest.input,
      matches: result.matches || [],
      matchProp: result.matchProp,
    };
    Manager.sendMessage("JSTerm:AutocompleteProperties", message);
  },

  



  _createSandbox: function JST__createSandbox()
  {
    this._sandboxLocation = this.window.location;
    this.sandbox = new Cu.Sandbox(this.window, {
      sandboxPrototype: this.window,
      wantXrays: false,
    });

    this.sandbox.console = this.console;

    JSTermHelper(this);
  },

  







  evalInSandbox: function JST_evalInSandbox(aString)
  {
    
    
    if (this._sandboxLocation !== this.window.location) {
      this._createSandbox();
    }

    
    if (aString.trim() == "help" || aString.trim() == "?") {
      aString = "help()";
    }

    let window = WebConsoleUtils.unwrap(this.sandbox.window);
    let $ = null, $$ = null;

    
    
    if (typeof window.$ == "function") {
      $ = this.sandbox.$;
      delete this.sandbox.$;
    }
    if (typeof window.$$ == "function") {
      $$ = this.sandbox.$$;
      delete this.sandbox.$$;
    }

    let result = Cu.evalInSandbox(aString, this.sandbox, "1.8",
                                  "Web Console", 1);

    if ($) {
      this.sandbox.$ = $;
    }
    if ($$) {
      this.sandbox.$$ = $$;
    }

    return result;
  },

  


  destroy: function JST_destroy()
  {
    for (let name in this._messageHandlers) {
      Manager.removeMessageHandler(name);
    }

    delete this.sandbox;
    delete this._sandboxLocation;
    delete this._messageHandlers;
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

      case "groupEnd":
        aRemoteMessage.argumentsToString =
          Array.map(aOriginalMessage.arguments || [],
                    this._formatObject.bind(this));
        break;

      case "log":
      case "info":
      case "warn":
      case "error":
      case "debug":
      case "dir": {
        aRemoteMessage.objectsCacheId = Manager.sequenceId;
        aRemoteMessage.argumentsToString = [];
        let mapFunction = function(aItem) {
          let formattedObject = this._formatObject(aItem);
          aRemoteMessage.argumentsToString.push(formattedObject);
          if (WebConsoleUtils.isObjectInspectable(aItem)) {
            return JSTerm.prepareObjectForRemote(aItem,
                                                 aRemoteMessage.objectsCacheId);
          }
          return formattedObject;
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

    if (!this.isCategoryAllowed(aScriptError.category)) {
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


  








  isCategoryAllowed: function CL_isCategoryAllowed(aCategory)
  {
    switch (aCategory) {
      case "XPConnect JavaScript":
      case "component javascript":
      case "chrome javascript":
      case "chrome registration":
      case "XBL":
      case "XBL Prototype Handler":
      case "XBL Content Sink":
      case "xbl javascript":
        return false;
    }

    return true;
  },

  







  getCachedMessages: function CL_getCachedMessages()
  {
    let innerWindowId = WebConsoleUtils.getInnerWindowId(Manager.window);
    let result = [];
    let errors = {};
    Services.console.getMessageArray(errors, {});

    (errors.value || []).forEach(function(aError) {
      if (!(aError instanceof Ci.nsIScriptError) ||
          aError.innerWindowID != innerWindowId ||
          !this.isCategoryAllowed(aError.category)) {
        return;
      }

      let remoteMessage = WebConsoleUtils.cloneObject(aError);
      remoteMessage._type = "PageError";
      result.push(remoteMessage);
    }, this);

    return result;
  },

  


  destroy: function CL_destroy()
  {
    Services.console.unregisterListener(this);
  },
};






const PR_UINT32_MAX = 4294967295;


const HTTP_MOVED_PERMANENTLY = 301;
const HTTP_FOUND = 302;
const HTTP_SEE_OTHER = 303;
const HTTP_TEMPORARY_REDIRECT = 307;


const RESPONSE_BODY_LIMIT = 1048576; 

















function NetworkResponseListener(aHttpActivity) {
  this.receivedData = "";
  this.httpActivity = aHttpActivity;
  this.bodySize = 0;
}

NetworkResponseListener.prototype = {
  QueryInterface:
    XPCOMUtils.generateQI([Ci.nsIStreamListener, Ci.nsIInputStreamCallback,
                           Ci.nsIRequestObserver, Ci.nsISupports]),

  




  _foundOpenResponse: false,

  



  sink: null,

  


  httpActivity: null,

  


  receivedData: null,

  


  bodySize: null,

  


  request: null,

  











  setAsyncListener: function NRL_setAsyncListener(aStream, aListener)
  {
    
    aStream.asyncWait(aListener, 0, 0, Services.tm.mainThread);
  },

  













  onDataAvailable:
  function NRL_onDataAvailable(aRequest, aContext, aInputStream, aOffset, aCount)
  {
    this._findOpenResponse();
    let data = NetUtil.readInputStreamToString(aInputStream, aCount);

    this.bodySize += aCount;

    if (!this.httpActivity.meta.discardResponseBody &&
        this.receivedData.length < RESPONSE_BODY_LIMIT) {
      this.receivedData += NetworkHelper.
                           convertToUnicode(data, aRequest.contentCharset);
    }
  },

  






  onStartRequest: function NRL_onStartRequest(aRequest)
  {
    this.request = aRequest;
    this._findOpenResponse();
    
    this.setAsyncListener(this.sink.inputStream, this);
  },

  





  onStopRequest: function NRL_onStopRequest()
  {
    this._findOpenResponse();
    this.sink.outputStream.close();
  },

  








  _findOpenResponse: function NRL__findOpenResponse()
  {
    if (!_alive || this._foundOpenResponse) {
      return;
    }

    let openResponse = null;

    for each (let item in NetworkMonitor.openResponses) {
      if (item.channel === this.httpActivity.channel) {
        openResponse = item;
        break;
      }
    }

    if (!openResponse) {
      return;
    }
    this._foundOpenResponse = true;

    let logResponse = this.httpActivity.log.entries[0].response;
    logResponse.headers = openResponse.headers;
    logResponse.httpVersion = openResponse.httpVersion;
    logResponse.status = openResponse.status;
    logResponse.statusText = openResponse.statusText;
    if (openResponse.cookies) {
      logResponse.cookies = openResponse.cookies;
    }

    delete NetworkMonitor.openResponses[openResponse.id];

    this.httpActivity.meta.stages.push("http-on-examine-response");
    NetworkMonitor.sendActivity(this.httpActivity);
  },

  





  onStreamClose: function NRL_onStreamClose()
  {
    if (!this.httpActivity) {
      return;
    }
    
    this.setAsyncListener(this.sink.inputStream, null);

    this._findOpenResponse();

    let meta = this.httpActivity.meta;
    let entry = this.httpActivity.log.entries[0];
    let request = entry.request;
    let response = entry.response;

    meta.stages.push("REQUEST_STOP");

    if (!meta.discardResponseBody && this.receivedData.length) {
      this._onComplete(this.receivedData);
    }
    else if (!meta.discardResponseBody && response.status == 304) {
      
      let charset = this.request.contentCharset || this.httpActivity.charset;
      NetworkHelper.loadFromCache(request.url, charset,
                                  this._onComplete.bind(this));
    }
    else {
      this._onComplete();
    }
  },

  







  _onComplete: function NRL__onComplete(aData)
  {
    let response = this.httpActivity.log.entries[0].response;

    try {
      response.bodySize = response.status != 304 ? this.request.contentLength : 0;
    }
    catch (ex) {
      response.bodySize = -1;
    }

    try {
      response.content = { mimeType: this.request.contentType };
    }
    catch (ex) {
      response.content = { mimeType: "" };
    }

    if (response.content.mimeType && this.request.contentCharset) {
      response.content.mimeType += "; charset=" + this.request.contentCharset;
    }

    response.content.size = this.bodySize || (aData || "").length;

    if (aData) {
      response.content.text = aData;
    }

    this.receivedData = "";

    if (_alive) {
      NetworkMonitor.sendActivity(this.httpActivity);
    }

    this.httpActivity.channel = null;
    this.httpActivity = null;
    this.sink = null;
    this.inputStream = null;
    this.request = null;
  },

  







  onInputStreamReady: function NRL_onInputStreamReady(aStream)
  {
    if (!(aStream instanceof Ci.nsIAsyncInputStream) || !this.httpActivity) {
      return;
    }

    let available = -1;
    try {
      
      available = aStream.available();
    }
    catch (ex) { }

    if (available != -1) {
      if (available != 0) {
        
        
        
        this.onDataAvailable(this.request, null, aStream, 0, available);
      }
      this.setAsyncListener(aStream, this);
    }
    else {
      this.onStreamClose();
    }
  },
};







let NetworkMonitor = {
  httpTransactionCodes: {
    0x5001: "REQUEST_HEADER",
    0x5002: "REQUEST_BODY_SENT",
    0x5003: "RESPONSE_START",
    0x5004: "RESPONSE_HEADER",
    0x5005: "RESPONSE_COMPLETE",
    0x5006: "TRANSACTION_CLOSE",

    0x804b0003: "STATUS_RESOLVING",
    0x804b000b: "STATUS_RESOLVED",
    0x804b0007: "STATUS_CONNECTING_TO",
    0x804b0004: "STATUS_CONNECTED_TO",
    0x804b0005: "STATUS_SENDING_TO",
    0x804b000a: "STATUS_WAITING_FOR",
    0x804b0006: "STATUS_RECEIVING_FROM"
  },

  harCreator: {
    name: Services.appinfo.name + " - Web Console",
    version: Services.appinfo.version,
  },

  
  
  responsePipeSegmentSize: null,

  



  get saveRequestAndResponseBodies() {
    return Manager.getPreference("NetworkMonitor.saveRequestAndResponseBodies");
  },

  openRequests: null,
  openResponses: null,
  progressListener: null,

  








  init: function NM_init(aMessage)
  {
    this.responsePipeSegmentSize = Services.prefs
                                   .getIntPref("network.buffer.cache.size");

    this.openRequests = {};
    this.openResponses = {};

    activityDistributor.addObserver(this);

    Services.obs.addObserver(this.httpResponseExaminer,
                             "http-on-examine-response", false);

    
    if (aMessage && aMessage.monitorFileActivity) {
      ConsoleProgressListener.startMonitor(ConsoleProgressListener
                                           .MONITOR_FILE_ACTIVITY);
    }
  },

  







  httpResponseExaminer: function NM_httpResponseExaminer(aSubject, aTopic)
  {
    
    
    
    

    if (!_alive || aTopic != "http-on-examine-response" ||
        !(aSubject instanceof Ci.nsIHttpChannel)) {
      return;
    }

    let channel = aSubject.QueryInterface(Ci.nsIHttpChannel);
    
    let win = NetworkHelper.getWindowForRequest(channel);
    if (!win || win.top !== Manager.window) {
      return;
    }

    let response = {
      id: Manager.sequenceId,
      channel: channel,
      headers: [],
      cookies: [],
    };

    let setCookieHeader = null;

    channel.visitResponseHeaders({
      visitHeader: function NM__visitHeader(aName, aValue) {
        let lowerName = aName.toLowerCase();
        if (lowerName == "set-cookie") {
          setCookieHeader = aValue;
        }
        response.headers.push({ name: aName, value: aValue });
      }
    });

    if (!response.headers.length) {
      return; 
    }

    if (setCookieHeader) {
      response.cookies = NetworkHelper.parseSetCookieHeader(setCookieHeader);
    }

    
    let httpVersionMaj = {};
    let httpVersionMin = {};

    channel.QueryInterface(Ci.nsIHttpChannelInternal);
    channel.getResponseVersion(httpVersionMaj, httpVersionMin);

    response.status = channel.responseStatus;
    response.statusText = channel.responseStatusText;
    response.httpVersion = "HTTP/" + httpVersionMaj.value + "." +
                                     httpVersionMin.value;

    NetworkMonitor.openResponses[response.id] = response;
  },

  











  observeActivity:
  function NM_observeActivity(aChannel, aActivityType, aActivitySubtype,
                              aTimestamp, aExtraSizeData, aExtraStringData)
  {
    if (!_alive ||
        aActivityType != activityDistributor.ACTIVITY_TYPE_HTTP_TRANSACTION &&
        aActivityType != activityDistributor.ACTIVITY_TYPE_SOCKET_TRANSPORT) {
      return;
    }

    if (!(aChannel instanceof Ci.nsIHttpChannel)) {
      return;
    }

    aChannel = aChannel.QueryInterface(Ci.nsIHttpChannel);

    if (aActivitySubtype ==
        activityDistributor.ACTIVITY_SUBTYPE_REQUEST_HEADER) {
      this._onRequestHeader(aChannel, aTimestamp, aExtraStringData);
      return;
    }

    
    
    let httpActivity = null;
    for each (let item in this.openRequests) {
      if (item.channel === aChannel) {
        httpActivity = item;
        break;
      }
    }

    if (!httpActivity) {
      return;
    }

    let transCodes = this.httpTransactionCodes;

    
    if (aActivitySubtype in transCodes) {
      let stage = transCodes[aActivitySubtype];
      if (stage in httpActivity.timings) {
        httpActivity.timings[stage].last = aTimestamp;
      }
      else {
        httpActivity.meta.stages.push(stage);
        httpActivity.timings[stage] = {
          first: aTimestamp,
          last: aTimestamp,
        };
      }
    }

    switch (aActivitySubtype) {
      case activityDistributor.ACTIVITY_SUBTYPE_REQUEST_BODY_SENT:
        this._onRequestBodySent(httpActivity);
        break;
      case activityDistributor.ACTIVITY_SUBTYPE_RESPONSE_HEADER:
        this._onResponseHeader(httpActivity, aExtraStringData);
        break;
      case activityDistributor.ACTIVITY_SUBTYPE_TRANSACTION_CLOSE:
        this._onTransactionClose(httpActivity);
        break;
      default:
        break;
    }
  },

  











  _onRequestHeader:
  function NM__onRequestHeader(aChannel, aTimestamp, aExtraStringData)
  {
    
    let win = NetworkHelper.getWindowForRequest(aChannel);
    if (!win || win.top !== Manager.window) {
      return;
    }

    let httpActivity = this.createActivityObject(aChannel);
    httpActivity.charset = win.document.characterSet; 
    httpActivity.meta.stages.push("REQUEST_HEADER"); 

    httpActivity.timings.REQUEST_HEADER = {
      first: aTimestamp,
      last: aTimestamp
    };

    let entry = httpActivity.log.entries[0];
    entry.startedDateTime = new Date(Math.round(aTimestamp / 1000)).toISOString();

    let request = httpActivity.log.entries[0].request;

    let cookieHeader = null;

    
    aChannel.visitRequestHeaders({
      visitHeader: function NM__visitHeader(aName, aValue)
      {
        if (aName == "Cookie") {
          cookieHeader = aValue;
        }
        request.headers.push({ name: aName, value: aValue });
      }
    });

    if (cookieHeader) {
      request.cookies = NetworkHelper.parseCookieHeader(cookieHeader);
    }

    
    let httpVersionMaj = {};
    let httpVersionMin = {};

    aChannel.QueryInterface(Ci.nsIHttpChannelInternal);
    aChannel.getRequestVersion(httpVersionMaj, httpVersionMin);

    request.httpVersion = "HTTP/" + httpVersionMaj.value + "." +
                                    httpVersionMin.value;

    request.headersSize = aExtraStringData.length;

    this._setupResponseListener(httpActivity);

    this.openRequests[httpActivity.id] = httpActivity;

    this.sendActivity(httpActivity);
  },

  














  createActivityObject: function NM_createActivityObject(aChannel)
  {
    return {
      hudId: Manager.hudId,
      id: Manager.sequenceId,
      channel: aChannel,
      charset: null, 
      meta: { 
        stages: [], 
        discardRequestBody: !this.saveRequestAndResponseBodies,
        discardResponseBody: !this.saveRequestAndResponseBodies,
      },
      timings: {}, 
      log: { 
        version: "1.2",
        creator: this.harCreator,
        
        entries: [{  
          connection: Manager.sequenceId, 
          startedDateTime: 0, 
          time: 0, 
          
          request: {
            method: aChannel.requestMethod,
            url: aChannel.URI.spec,
            httpVersion: "", 
            headers: [], 
            cookies: [], 
            queryString: [], 
            headersSize: -1, 
            bodySize: -1, 
            postData: null, 
          },
          response: {
            status: 0, 
            statusText: "", 
            httpVersion: "", 
            headers: [], 
            cookies: [], 
            content: null, 
            redirectURL: "", 
            headersSize: -1, 
            bodySize: -1, 
          },
          timings: {}, 
        }],
      },
    };
  },

  







  _setupResponseListener: function NM__setupResponseListener(aHttpActivity)
  {
    let channel = aHttpActivity.channel;
    channel.QueryInterface(Ci.nsITraceableChannel);

    
    
    
    
    let sink = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);

    
    
    sink.init(false, false, this.responsePipeSegmentSize, PR_UINT32_MAX, null);

    
    let newListener = new NetworkResponseListener(aHttpActivity);

    
    newListener.inputStream = sink.inputStream;
    newListener.sink = sink;

    let tee = Cc["@mozilla.org/network/stream-listener-tee;1"].
              createInstance(Ci.nsIStreamListenerTee);

    let originalListener = channel.setNewListener(tee);

    tee.init(originalListener, sink.outputStream, newListener);
  },

  









  sendActivity: function NM_sendActivity(aHttpActivity)
  {
    Manager.sendMessage("WebConsole:NetworkActivity", {
      meta: aHttpActivity.meta,
      log: aHttpActivity.log,
    });
  },

  







  _onRequestBodySent: function NM__onRequestBodySent(aHttpActivity)
  {
    if (aHttpActivity.meta.discardRequestBody) {
      return;
    }

    let request = aHttpActivity.log.entries[0].request;

    let sentBody = NetworkHelper.
                   readPostTextFromRequest(aHttpActivity.channel,
                                           aHttpActivity.charset);

    if (!sentBody && request.url == Manager.window.location.href) {
      
      
      
      
      
      
      
      sentBody = NetworkHelper.readPostTextFromPage(docShell,
                                                    aHttpActivity.charset);
    }
    if (!sentBody) {
      return;
    }

    request.postData = {
      mimeType: "", 
      params: [],  
      text: sentBody,
    };

    request.bodySize = sentBody.length;

    this.sendActivity(aHttpActivity);
  },

  









  _onResponseHeader:
  function NM__onResponseHeader(aHttpActivity, aExtraStringData)
  {
    
    
    
    
    
    
    
    
    
    

    let response = aHttpActivity.log.entries[0].response;

    let headers = aExtraStringData.split(/\r\n|\n|\r/);
    let statusLine = headers.shift();

    let statusLineArray = statusLine.split(" ");
    response.httpVersion = statusLineArray.shift();
    response.status = statusLineArray.shift();
    response.statusText = statusLineArray.join(" ");
    response.headersSize = aExtraStringData.length;

    
    switch (parseInt(response.status)) {
      case HTTP_MOVED_PERMANENTLY:
      case HTTP_FOUND:
      case HTTP_SEE_OTHER:
      case HTTP_TEMPORARY_REDIRECT:
        aHttpActivity.meta.discardResponseBody = true;
        break;
    }

    this.sendActivity(aHttpActivity);
  },

  








  _onTransactionClose: function NM__onTransactionClose(aHttpActivity)
  {
    this._setupHarTimings(aHttpActivity);
    this.sendActivity(aHttpActivity);
    delete this.openRequests[aHttpActivity.id];
  },

  








  _setupHarTimings: function NM__setupHarTimings(aHttpActivity)
  {
    let timings = aHttpActivity.timings;
    let entry = aHttpActivity.log.entries[0];
    let harTimings = entry.timings;

    
    harTimings.blocked = -1;

    
    
    harTimings.dns = timings.STATUS_RESOLVING && timings.STATUS_RESOLVED ?
                     timings.STATUS_RESOLVED.last -
                     timings.STATUS_RESOLVING.first : -1;

    if (timings.STATUS_CONNECTING_TO && timings.STATUS_CONNECTED_TO) {
      harTimings.connect = timings.STATUS_CONNECTED_TO.last -
                           timings.STATUS_CONNECTING_TO.first;
    }
    else if (timings.STATUS_SENDING_TO) {
      harTimings.connect = timings.STATUS_SENDING_TO.first -
                           timings.REQUEST_HEADER.first;
    }
    else {
      harTimings.connect = -1;
    }

    if ((timings.STATUS_WAITING_FOR || timings.STATUS_RECEIVING_FROM) &&
        (timings.STATUS_CONNECTED_TO || timings.STATUS_SENDING_TO)) {
      harTimings.send = (timings.STATUS_WAITING_FOR ||
                         timings.STATUS_RECEIVING_FROM).first -
                        (timings.STATUS_CONNECTED_TO ||
                         timings.STATUS_SENDING_TO).last;
    }
    else {
      harTimings.send = -1;
    }

    if (timings.RESPONSE_START) {
      harTimings.wait = timings.RESPONSE_START.first -
                        (timings.REQUEST_BODY_SENT ||
                         timings.STATUS_SENDING_TO).last;
    }
    else {
      harTimings.wait = -1;
    }

    if (timings.RESPONSE_START && timings.RESPONSE_COMPLETE) {
      harTimings.receive = timings.RESPONSE_COMPLETE.last -
                           timings.RESPONSE_START.first;
    }
    else {
      harTimings.receive = -1;
    }

    entry.time = 0;
    for (let timing in harTimings) {
      let time = Math.max(Math.round(harTimings[timing] / 1000), -1);
      harTimings[timing] = time;
      if (time > -1) {
        entry.time += time;
      }
    }
  },

  



  destroy: function NM_destroy()
  {
    Services.obs.removeObserver(this.httpResponseExaminer,
                                "http-on-examine-response");

    activityDistributor.removeObserver(this);

    ConsoleProgressListener.stopMonitor(ConsoleProgressListener
                                        .MONITOR_FILE_ACTIVITY);

    delete this.openRequests;
    delete this.openResponses;
  },
};














let ConsoleProgressListener = {
  



  MONITOR_FILE_ACTIVITY: 1,

  



  MONITOR_LOCATION_CHANGE: 2,

  




  _fileActivity: false,

  




  _locationChange: false,

  




  _initialized: false,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference]),

  



  _init: function CPL__init()
  {
    if (this._initialized) {
      return;
    }

    this._initialized = true;
    let webProgress = docShell.QueryInterface(Ci.nsIWebProgress);
    webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_STATE_ALL);
  },

  










  startMonitor: function CPL_startMonitor(aMonitor)
  {
    switch (aMonitor) {
      case this.MONITOR_FILE_ACTIVITY:
        this._fileActivity = true;
        break;
      case this.MONITOR_LOCATION_CHANGE:
        this._locationChange = true;
        break;
      default:
        throw new Error("HUDService-content: unknown monitor type " +
                        aMonitor + " for the ConsoleProgressListener!");
    }
    this._init();
  },

  






  stopMonitor: function CPL_stopMonitor(aMonitor)
  {
    switch (aMonitor) {
      case this.MONITOR_FILE_ACTIVITY:
        this._fileActivity = false;
        break;
      case this.MONITOR_LOCATION_CHANGE:
        this._locationChange = false;
        break;
      default:
        throw new Error("HUDService-content: unknown monitor type " +
                        aMonitor + " for the ConsoleProgressListener!");
    }

    if (!this._fileActivity && !this._locationChange) {
      this.destroy();
    }
  },

  onStateChange:
  function CPL_onStateChange(aProgress, aRequest, aState, aStatus)
  {
    if (!_alive) {
      return;
    }

    if (this._fileActivity) {
      this._checkFileActivity(aProgress, aRequest, aState, aStatus);
    }

    if (this._locationChange) {
      this._checkLocationChange(aProgress, aRequest, aState, aStatus);
    }
  },

  





  _checkFileActivity:
  function CPL__checkFileActivity(aProgress, aRequest, aState, aStatus)
  {
    if (!(aState & Ci.nsIWebProgressListener.STATE_START)) {
      return;
    }

    let uri = null;
    if (aRequest instanceof Ci.imgIRequest) {
      let imgIRequest = aRequest.QueryInterface(Ci.imgIRequest);
      uri = imgIRequest.URI;
    }
    else if (aRequest instanceof Ci.nsIChannel) {
      let nsIChannel = aRequest.QueryInterface(Ci.nsIChannel);
      uri = nsIChannel.URI;
    }

    if (!uri || !uri.schemeIs("file") && !uri.schemeIs("ftp")) {
      return;
    }

    Manager.sendMessage("WebConsole:FileActivity", {uri: uri.spec});
  },

  





  _checkLocationChange:
  function CPL__checkLocationChange(aProgress, aRequest, aState, aStatus)
  {
    let isStart = aState & Ci.nsIWebProgressListener.STATE_START;
    let isStop = aState & Ci.nsIWebProgressListener.STATE_STOP;
    let isNetwork = aState & Ci.nsIWebProgressListener.STATE_IS_NETWORK;
    let isWindow = aState & Ci.nsIWebProgressListener.STATE_IS_WINDOW;

    
    if (!isNetwork || !isWindow ||
        aProgress.DOMWindow != Manager.window) {
      return;
    }

    if (isStart && aRequest instanceof Ci.nsIChannel) {
      this.sendLocation(aRequest.URI.spec, "");
    }
    else if (isStop) {
      this.sendLocation(Manager.window.location.href,
                        Manager.window.document.title);
    }
  },

  onLocationChange: function() {},
  onStatusChange: function() {},
  onProgressChange: function() {},
  onSecurityChange: function() {},

  









  sendLocation: function CPL_sendLocation(aLocation, aTitle)
  {
    let message = {
      "location": aLocation,
      "title": aTitle,
    };
    Manager.sendMessage("WebConsole:LocationChange", message);
  },

  


  destroy: function CPL_destroy()
  {
    if (!this._initialized) {
      return;
    }

    this._initialized = false;
    this._fileActivity = false;
    this._locationChange = false;
    let webProgress = docShell.QueryInterface(Ci.nsIWebProgress);
    webProgress.removeProgressListener(this);
  },
};

Manager.init();
})();
