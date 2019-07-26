





"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PageErrorListener",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ConsoleAPIListener",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ConsoleProgressListener",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "JSTermHelpers",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "JSPropertyProvider",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetworkMonitor",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ConsoleAPIStorage",
                                  "resource://gre/modules/ConsoleAPIStorage.jsm");












function WebConsoleActor(aConnection, aParentActor)
{
  this.conn = aConnection;

  if (aParentActor instanceof BrowserTabActor &&
      aParentActor.browser instanceof Ci.nsIDOMWindow) {
    this._window = aParentActor.browser;
  }
  else if (aParentActor instanceof BrowserTabActor &&
           aParentActor.browser instanceof Ci.nsIDOMElement) {
    this._window = aParentActor.browser.contentWindow;
  }
  else {
    this._window = Services.wm.getMostRecentWindow("navigator:browser");
    this._isGlobalActor = true;
  }

  this._actorPool = new ActorPool(this.conn);
  this.conn.addActorPool(this._actorPool);

  this._prefs = {};
}

WebConsoleActor.prototype =
{
  




  _isGlobalActor: false,

  





  _actorPool: null,

  




  _prefs: null,

  





  _sandboxWindowId: 0,

  



  sandbox: null,

  



  conn: null,

  



  get window() this._window,

  _window: null,

  



  pageErrorListener: null,

  


  consoleAPIListener: null,

  


  networkMonitor: null,

  


  consoleProgressListener: null,

  



  get saveRequestAndResponseBodies()
    this._prefs["NetworkMonitor.saveRequestAndResponseBodies"],

  actorPrefix: "console",

  grip: function WCA_grip()
  {
    return { actor: this.actorID };
  },

  hasNativeConsoleAPI: BrowserTabActor.prototype.hasNativeConsoleAPI,

  


  disconnect: function WCA_disconnect()
  {
    if (this.pageErrorListener) {
      this.pageErrorListener.destroy();
      this.pageErrorListener = null;
    }
    if (this.consoleAPIListener) {
      this.consoleAPIListener.destroy();
      this.consoleAPIListener = null;
    }
    if (this.networkMonitor) {
      this.networkMonitor.destroy();
      this.networkMonitor = null;
    }
    if (this.consoleProgressListener) {
      this.consoleProgressListener.destroy();
      this.consoleProgressListener = null;
    }
    this.conn.removeActorPool(this._actorPool);
    this._actorPool = null;
    this.sandbox = null;
    this._sandboxWindowId = 0;
    this.conn = this._window = null;
  },

  






  createValueGrip: function WCA_createValueGrip(aValue)
  {
    return WebConsoleUtils.createValueGrip(aValue,
                                           this.createObjectActor.bind(this));
  },

  







  createObjectActor: function WCA_createObjectActor(aObject)
  {
    if (typeof aObject == "string") {
      return this.createStringGrip(aObject);
    }

    
    
    let obj = WebConsoleUtils.unwrap(aObject);
    let actor = new WebConsoleObjectActor(obj, this);
    this._actorPool.addActor(actor);
    return actor.grip();
  },

  









  createStringGrip: function WCA_createStringGrip(aString)
  {
    if (aString.length >= DebuggerServer.LONG_STRING_LENGTH) {
      let actor = new LongStringActor(aString, this);
      this._actorPool.addActor(actor);
      return actor.grip();
    }
    return aString;
  },

  





  getActorByID: function WCA_getActorByID(aActorID)
  {
    return this._actorPool.get(aActorID);
  },

  





  releaseActor: function WCA_releaseActor(aActor)
  {
    this._actorPool.removeActor(aActor.actorID);
  },

  
  
  

  







  onStartListeners: function WCA_onStartListeners(aRequest)
  {
    let startedListeners = [];
    let window = !this._isGlobalActor ? this.window : null;

    while (aRequest.listeners.length > 0) {
      let listener = aRequest.listeners.shift();
      switch (listener) {
        case "PageError":
          if (!this.pageErrorListener) {
            this.pageErrorListener =
              new PageErrorListener(window, this);
            this.pageErrorListener.init();
          }
          startedListeners.push(listener);
          break;
        case "ConsoleAPI":
          if (!this.consoleAPIListener) {
            this.consoleAPIListener =
              new ConsoleAPIListener(window, this);
            this.consoleAPIListener.init();
          }
          startedListeners.push(listener);
          break;
        case "NetworkActivity":
          if (!this.networkMonitor) {
            this.networkMonitor =
              new NetworkMonitor(window, this);
            this.networkMonitor.init();
          }
          startedListeners.push(listener);
          break;
        case "FileActivity":
          if (!this.consoleProgressListener) {
            this.consoleProgressListener =
              new ConsoleProgressListener(this.window, this);
          }
          this.consoleProgressListener.startMonitor(this.consoleProgressListener.
                                                    MONITOR_FILE_ACTIVITY);
          startedListeners.push(listener);
          break;
      }
    }
    return {
      startedListeners: startedListeners,
      nativeConsoleAPI: this.hasNativeConsoleAPI(this.window),
    };
  },

  








  onStopListeners: function WCA_onStopListeners(aRequest)
  {
    let stoppedListeners = [];

    
    
    let toDetach = aRequest.listeners ||
                   ["PageError", "ConsoleAPI", "NetworkActivity",
                    "FileActivity"];

    while (toDetach.length > 0) {
      let listener = toDetach.shift();
      switch (listener) {
        case "PageError":
          if (this.pageErrorListener) {
            this.pageErrorListener.destroy();
            this.pageErrorListener = null;
          }
          stoppedListeners.push(listener);
          break;
        case "ConsoleAPI":
          if (this.consoleAPIListener) {
            this.consoleAPIListener.destroy();
            this.consoleAPIListener = null;
          }
          stoppedListeners.push(listener);
          break;
        case "NetworkActivity":
          if (this.networkMonitor) {
            this.networkMonitor.destroy();
            this.networkMonitor = null;
          }
          stoppedListeners.push(listener);
          break;
        case "FileActivity":
          if (this.consoleProgressListener) {
            this.consoleProgressListener.stopMonitor(this.consoleProgressListener.
                                                     MONITOR_FILE_ACTIVITY);
          }
          stoppedListeners.push(listener);
          break;
      }
    }

    return { stoppedListeners: stoppedListeners };
  },

  









  onGetCachedMessages: function WCA_onGetCachedMessages(aRequest)
  {
    let types = aRequest.messageTypes;
    if (!types) {
      return {
        error: "missingParameter",
        message: "The messageTypes parameter is missing.",
      };
    }

    let messages = [];

    while (types.length > 0) {
      let type = types.shift();
      switch (type) {
        case "ConsoleAPI":
          if (this.consoleAPIListener) {
            let cache = this.consoleAPIListener.getCachedMessages();
            cache.forEach(function(aMessage) {
              let message = this.prepareConsoleMessageForRemote(aMessage);
              message._type = type;
              messages.push(message);
            }, this);
          }
          break;
        case "PageError":
          if (this.pageErrorListener) {
            let cache = this.pageErrorListener.getCachedMessages();
            cache.forEach(function(aMessage) {
              let message = this.preparePageErrorForRemote(aMessage);
              message._type = type;
              messages.push(message);
            }, this);
          }
          break;
      }
    }

    messages.sort(function(a, b) { return a.timeStamp - b.timeStamp; });

    return {
      from: this.actorID,
      messages: messages,
    };
  },

  








  onEvaluateJS: function WCA_onEvaluateJS(aRequest)
  {
    let input = aRequest.text;
    let result, error = null;
    let timestamp;

    this.helperResult = null;
    this.evalInput = input;
    try {
      timestamp = Date.now();
      result = this.evalInSandbox(input);
    }
    catch (ex) {
      error = ex;
    }

    let helperResult = this.helperResult;
    delete this.helperResult;
    delete this.evalInput;

    return {
      from: this.actorID,
      input: input,
      result: this.createValueGrip(result),
      timestamp: timestamp,
      error: error,
      errorMessage: error ? String(error) : null,
      helperResult: helperResult,
    };
  },

  







  onAutocomplete: function WCA_onAutocomplete(aRequest)
  {
    let result = JSPropertyProvider(this.window, aRequest.text) || {};
    return {
      from: this.actorID,
      matches: result.matches || [],
      matchProp: result.matchProp,
    };
  },

  


  onClearMessagesCache: function WCA_onClearMessagesCache()
  {
    
    let windowId = !this._isGlobalActor ?
                   WebConsoleUtils.getInnerWindowId(this.window) : null;
    ConsoleAPIStorage.clearEvents(windowId);
    return {};
  },

  





  onSetPreferences: function WCA_onSetPreferences(aRequest)
  {
    for (let key in aRequest.preferences) {
      this._prefs[key] = aRequest.preferences[key];
    }
    return { updated: Object.keys(aRequest.preferences) };
  },

  
  
  

  



  _createSandbox: function WCA__createSandbox()
  {
    this._sandboxWindowId = WebConsoleUtils.getInnerWindowId(this.window);
    this.sandbox = new Cu.Sandbox(this.window, {
      sandboxPrototype: this.window,
      wantXrays: false,
    });

    this.sandbox.console = this.window.console;

    JSTermHelpers(this);
  },

  







  evalInSandbox: function WCA_evalInSandbox(aString)
  {
    
    
    if (this._sandboxWindowId !== WebConsoleUtils.getInnerWindowId(this.window)) {
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

  
  
  

  






  onPageError: function WCA_onPageError(aPageError)
  {
    let packet = {
      from: this.actorID,
      type: "pageError",
      pageError: this.preparePageErrorForRemote(aPageError),
    };
    this.conn.send(packet);
  },

  







  preparePageErrorForRemote: function WCA_preparePageErrorForRemote(aPageError)
  {
    return {
      message: aPageError.message,
      errorMessage: aPageError.errorMessage,
      sourceName: aPageError.sourceName,
      lineText: aPageError.sourceLine,
      lineNumber: aPageError.lineNumber,
      columnNumber: aPageError.columnNumber,
      category: aPageError.category,
      timeStamp: aPageError.timeStamp,
      warning: !!(aPageError.flags & aPageError.warningFlag),
      error: !!(aPageError.flags & aPageError.errorFlag),
      exception: !!(aPageError.flags & aPageError.exceptionFlag),
      strict: !!(aPageError.flags & aPageError.strictFlag),
    };
  },

  







  onConsoleAPICall: function WCA_onConsoleAPICall(aMessage)
  {
    let packet = {
      from: this.actorID,
      type: "consoleAPICall",
      message: this.prepareConsoleMessageForRemote(aMessage),
    };
    this.conn.send(packet);
  },

  












  onNetworkEvent: function WCA_onNetworkEvent(aEvent)
  {
    let actor = new NetworkEventActor(aEvent, this);
    this._actorPool.addActor(actor);

    let packet = {
      from: this.actorID,
      type: "networkEvent",
      eventActor: actor.grip(),
    };

    this.conn.send(packet);

    return actor;
  },

  







  onFileActivity: function WCA_onFileActivity(aFileURI)
  {
    let packet = {
      from: this.actorID,
      type: "fileActivity",
      uri: aFileURI,
    };
    this.conn.send(packet);
  },

  
  
  

  








  prepareConsoleMessageForRemote:
  function WCA_prepareConsoleMessageForRemote(aMessage)
  {
    let result = WebConsoleUtils.cloneObject(aMessage);
    delete result.wrappedJSObject;

    result.arguments = Array.map(aMessage.arguments || [],
      function(aObj) {
        return this.createValueGrip(aObj);
      }, this);

    if (result.level == "dir") {
      result.objectProperties = [];
      let first = result.arguments[0];
      if (typeof first == "object" && first && first.inspectable) {
        let actor = this.getActorByID(first.actor);
        result.objectProperties = actor.onInspectProperties().properties;
      }
    }

    return result;
  },

  





  chromeWindow: function WCA_chromeWindow()
  {
    return this.window.QueryInterface(Ci.nsIInterfaceRequestor)
           .getInterface(Ci.nsIWebNavigation).QueryInterface(Ci.nsIDocShell)
           .chromeEventHandler.ownerDocument.defaultView;
  },
};

WebConsoleActor.prototype.requestTypes =
{
  startListeners: WebConsoleActor.prototype.onStartListeners,
  stopListeners: WebConsoleActor.prototype.onStopListeners,
  getCachedMessages: WebConsoleActor.prototype.onGetCachedMessages,
  evaluateJS: WebConsoleActor.prototype.onEvaluateJS,
  autocomplete: WebConsoleActor.prototype.onAutocomplete,
  clearMessagesCache: WebConsoleActor.prototype.onClearMessagesCache,
  setPreferences: WebConsoleActor.prototype.onSetPreferences,
};










function WebConsoleObjectActor(aObj, aWebConsoleActor)
{
  this.obj = aObj;
  this.parent = aWebConsoleActor;
}

WebConsoleObjectActor.prototype =
{
  actorPrefix: "consoleObj",

  


  grip: function WCOA_grip()
  {
    let grip = WebConsoleUtils.getObjectGrip(this.obj);
    grip.actor = this.actorID;
    grip.displayString = this.parent.createStringGrip(grip.displayString);
    return grip;
  },

  


  release: function WCOA_release()
  {
    this.parent.releaseActor(this);
    this.parent = this.obj = null;
  },

  






  onInspectProperties: function WCOA_onInspectProperties()
  {
    let createObjectActor = this.parent.createObjectActor.bind(this.parent);
    let props = WebConsoleUtils.inspectObject(this.obj, createObjectActor);
    return {
      from: this.actorID,
      properties: props,
    };
  },

  


  onRelease: function WCOA_onRelease()
  {
    this.release();
    return {};
  },
};

WebConsoleObjectActor.prototype.requestTypes =
{
  "inspectProperties": WebConsoleObjectActor.prototype.onInspectProperties,
  "release": WebConsoleObjectActor.prototype.onRelease,
};











function NetworkEventActor(aNetworkEvent, aWebConsoleActor)
{
  this.parent = aWebConsoleActor;
  this.conn = this.parent.conn;

  this._startedDateTime = aNetworkEvent.startedDateTime;

  this._request = {
    method: aNetworkEvent.method,
    url: aNetworkEvent.url,
    httpVersion: aNetworkEvent.httpVersion,
    headers: [],
    cookies: [],
    headersSize: aNetworkEvent.headersSize,
    postData: {},
  };

  this._response = {
    headers: [],
    cookies: [],
    content: {},
  };

  this._timings = {};
  this._longStringActors = new Set();

  this._discardRequestBody = aNetworkEvent.discardRequestBody;
  this._discardResponseBody = aNetworkEvent.discardResponseBody;
}

NetworkEventActor.prototype =
{
  _request: null,
  _response: null,
  _timings: null,
  _longStringActors: null,

  actorPrefix: "netEvent",

  


  grip: function NEA_grip()
  {
    return {
      actor: this.actorID,
      startedDateTime: this._startedDateTime,
      url: this._request.url,
      method: this._request.method,
    };
  },

  


  release: function NEA_release()
  {
    for (let grip of this._longStringActors) {
      let actor = this.parent.getActorByID(grip.actor);
      if (actor) {
        this.parent.releaseActor(actor);
      }
    }
    this._longStringActors = new Set();
    this.parent.releaseActor(this);
  },

  


  onRelease: function NEA_onRelease()
  {
    this.release();
    return {};
  },

  





  onGetRequestHeaders: function NEA_onGetRequestHeaders()
  {
    return {
      from: this.actorID,
      headers: this._request.headers,
      headersSize: this._request.headersSize,
    };
  },

  





  onGetRequestCookies: function NEA_onGetRequestCookies()
  {
    return {
      from: this.actorID,
      cookies: this._request.cookies,
    };
  },

  





  onGetRequestPostData: function NEA_onGetRequestPostData()
  {
    return {
      from: this.actorID,
      postData: this._request.postData,
      postDataDiscarded: this._discardRequestBody,
    };
  },

  





  onGetResponseHeaders: function NEA_onGetResponseHeaders()
  {
    return {
      from: this.actorID,
      headers: this._response.headers,
      headersSize: this._response.headersSize,
    };
  },

  





  onGetResponseCookies: function NEA_onGetResponseCookies()
  {
    return {
      from: this.actorID,
      cookies: this._response.cookies,
    };
  },

  





  onGetResponseContent: function NEA_onGetResponseContent()
  {
    return {
      from: this.actorID,
      content: this._response.content,
      contentDiscarded: this._discardResponseBody,
    };
  },

  





  onGetEventTimings: function NEA_onGetEventTimings()
  {
    return {
      from: this.actorID,
      timings: this._timings,
      totalTime: this._totalTime,
    };
  },

  



  





  addRequestHeaders: function NEA_addRequestHeaders(aHeaders)
  {
    this._request.headers = aHeaders;
    this._prepareHeaders(aHeaders);

    let packet = {
      from: this.actorID,
      type: "networkEventUpdate",
      updateType: "requestHeaders",
      headers: aHeaders.length,
      headersSize: this._request.headersSize,
    };

    this.conn.send(packet);
  },

  





  addRequestCookies: function NEA_addRequestCookies(aCookies)
  {
    this._request.cookies = aCookies;
    this._prepareHeaders(aCookies);

    let packet = {
      from: this.actorID,
      type: "networkEventUpdate",
      updateType: "requestCookies",
      cookies: aCookies.length,
    };

    this.conn.send(packet);
  },

  





  addRequestPostData: function NEA_addRequestPostData(aPostData)
  {
    this._request.postData = aPostData;
    aPostData.text = this.parent.createStringGrip(aPostData.text);
    if (typeof aPostData.text == "object") {
      this._longStringActors.add(aPostData.text);
    }

    let packet = {
      from: this.actorID,
      type: "networkEventUpdate",
      updateType: "requestPostData",
      dataSize: aPostData.text.length,
      discardRequestBody: this._discardRequestBody,
    };

    this.conn.send(packet);
  },

  





  addResponseStart: function NEA_addResponseStart(aInfo)
  {
    this._response.httpVersion = aInfo.httpVersion;
    this._response.status = aInfo.status;
    this._response.statusText = aInfo.statusText;
    this._response.headersSize = aInfo.headersSize;
    this._discardResponseBody = aInfo.discardResponseBody;

    let packet = {
      from: this.actorID,
      type: "networkEventUpdate",
      updateType: "responseStart",
      response: aInfo,
    };

    this.conn.send(packet);
  },

  





  addResponseHeaders: function NEA_addResponseHeaders(aHeaders)
  {
    this._response.headers = aHeaders;
    this._prepareHeaders(aHeaders);

    let packet = {
      from: this.actorID,
      type: "networkEventUpdate",
      updateType: "responseHeaders",
      headers: aHeaders.length,
      headersSize: this._response.headersSize,
    };

    this.conn.send(packet);
  },

  





  addResponseCookies: function NEA_addResponseCookies(aCookies)
  {
    this._response.cookies = aCookies;
    this._prepareHeaders(aCookies);

    let packet = {
      from: this.actorID,
      type: "networkEventUpdate",
      updateType: "responseCookies",
      cookies: aCookies.length,
    };

    this.conn.send(packet);
  },

  







  addResponseContent:
  function NEA_addResponseContent(aContent, aDiscardedResponseBody)
  {
    this._response.content = aContent;
    aContent.text = this.parent.createStringGrip(aContent.text);
    if (typeof aContent.text == "object") {
      this._longStringActors.add(aContent.text);
    }

    let packet = {
      from: this.actorID,
      type: "networkEventUpdate",
      updateType: "responseContent",
      mimeType: aContent.mimeType,
      contentSize: aContent.text.length,
      discardResponseBody: aDiscardedResponseBody,
    };

    this.conn.send(packet);
  },

  







  addEventTimings: function NEA_addEventTimings(aTotal, aTimings)
  {
    this._totalTime = aTotal;
    this._timings = aTimings;

    let packet = {
      from: this.actorID,
      type: "networkEventUpdate",
      updateType: "eventTimings",
      totalTime: aTotal,
    };

    this.conn.send(packet);
  },

  






  _prepareHeaders: function NEA__prepareHeaders(aHeaders)
  {
    for (let header of aHeaders) {
      header.value = this.parent.createStringGrip(header.value);
      if (typeof header.value == "object") {
        this._longStringActors.add(header.value);
      }
    }
  },
};

NetworkEventActor.prototype.requestTypes =
{
  "release": NetworkEventActor.prototype.onRelease,
  "getRequestHeaders": NetworkEventActor.prototype.onGetRequestHeaders,
  "getRequestCookies": NetworkEventActor.prototype.onGetRequestCookies,
  "getRequestPostData": NetworkEventActor.prototype.onGetRequestPostData,
  "getResponseHeaders": NetworkEventActor.prototype.onGetResponseHeaders,
  "getResponseCookies": NetworkEventActor.prototype.onGetResponseCookies,
  "getResponseContent": NetworkEventActor.prototype.onGetResponseContent,
  "getEventTimings": NetworkEventActor.prototype.onGetEventTimings,
};

