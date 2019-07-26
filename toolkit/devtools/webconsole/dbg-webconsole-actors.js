





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

XPCOMUtils.defineLazyModuleGetter(this, "JSTermHelpers",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "JSPropertyProvider",
                                  "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ConsoleAPIStorage",
                                  "resource://gre/modules/ConsoleAPIStorage.jsm");












function WebConsoleActor(aConnection, aTabActor)
{
  this.conn = aConnection;
  this._browser = aTabActor.browser;

  this._objectActorsPool = new ActorPool(this.conn);
  this.conn.addActorPool(this._objectActorsPool);
}

WebConsoleActor.prototype =
{
  




  _browser: null,

  






  _objectActorsPool: null,

  





  _sandboxLocation: null,

  



  sandbox: null,

  



  conn: null,

  



  get window() this._browser.contentWindow,

  



  pageErrorListener: null,

  


  consoleAPIListener: null,

  actorPrefix: "console",

  grip: function WCA_grip()
  {
    return { actor: this.actorID };
  },

  






  hasNativeConsoleAPI: function WCA_hasNativeConsoleAPI()
  {
    let isNative = false;
    try {
      let consoleObject = WebConsoleUtils.unwrap(this.window).console;
      isNative = "__mozillaConsole__" in consoleObject;
    }
    catch (ex) { }
    return isNative;
  },

  


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
    this.conn.removeActorPool(this._objectActorsPool);
    this._objectActorsPool = null;
    this._sandboxLocation = this.sandbox = null;
    this.conn = this._browser = null;
  },

  






  createValueGrip: function WCA_createValueGrip(aValue)
  {
    return WebConsoleUtils.createValueGrip(aValue,
                                           this.createObjectActor.bind(this));
  },

  







  createObjectActor: function WCA_createObjectActor(aObject)
  {
    
    
    let obj = WebConsoleUtils.unwrap(aObject);
    let actor = new WebConsoleObjectActor(obj, this);
    this._objectActorsPool.addActor(actor);
    return actor.grip();
  },

  





  getObjectActorByID: function WCA_getObjectActorByID(aActorID)
  {
    return this._objectActorsPool.get(aActorID);
  },

  





  releaseObject: function WCA_releaseObject(aActor)
  {
    this._objectActorsPool.removeActor(aActor.actorID);
  },

  







  onStartListeners: function WCA_onStartListeners(aRequest)
  {
    let startedListeners = [];

    while (aRequest.listeners.length > 0) {
      let listener = aRequest.listeners.shift();
      switch (listener) {
        case "PageError":
          if (!this.pageErrorListener) {
            this.pageErrorListener =
              new PageErrorListener(this.window, this);
            this.pageErrorListener.init();
          }
          startedListeners.push(listener);
          break;
        case "ConsoleAPI":
          if (!this.consoleAPIListener) {
            this.consoleAPIListener =
              new ConsoleAPIListener(this.window, this);
            this.consoleAPIListener.init();
          }
          startedListeners.push(listener);
          break;
      }
    }
    return {
      startedListeners: startedListeners,
      nativeConsoleAPI: this.hasNativeConsoleAPI(),
    };
  },

  








  onStopListeners: function WCA_onStopListeners(aRequest)
  {
    let stoppedListeners = [];

    
    
    let toDetach = aRequest.listeners || ["PageError", "ConsoleAPI"];

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
    
    let windowId = WebConsoleUtils.getInnerWindowId(this.window);
    ConsoleAPIStorage.clearEvents(windowId);
    return {};
  },

  



  _createSandbox: function WCA__createSandbox()
  {
    this._sandboxLocation = this.window.location;
    this.sandbox = new Cu.Sandbox(this.window, {
      sandboxPrototype: this.window,
      wantXrays: false,
    });

    this.sandbox.console = this.window.console;

    JSTermHelpers(this);
  },

  







  evalInSandbox: function WCA_evalInSandbox(aString)
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

  








  prepareConsoleMessageForRemote:
  function WCA_prepareConsoleMessageForRemote(aMessage)
  {
    let result = {
      level: aMessage.level,
      filename: aMessage.filename,
      lineNumber: aMessage.lineNumber,
      functionName: aMessage.functionName,
      timeStamp: aMessage.timeStamp,
    };

    switch (result.level) {
      case "trace":
      case "group":
      case "groupCollapsed":
      case "time":
      case "timeEnd":
        result.arguments = aMessage.arguments;
        break;
      default:
        result.arguments = Array.map(aMessage.arguments || [],
          function(aObj) {
            return this.createValueGrip(aObj);
          }, this);

        if (result.level == "dir") {
          result.objectProperties = [];
          let first = result.arguments[0];
          if (typeof first == "object" && first && first.inspectable) {
            let actor = this.getObjectActorByID(first.actor);
            result.objectProperties = actor.onInspectProperties().properties;
          }
        }
        break;
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
    return grip;
  },

  


  release: function WCOA_release()
  {
    this.parent.releaseObject(this);
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

