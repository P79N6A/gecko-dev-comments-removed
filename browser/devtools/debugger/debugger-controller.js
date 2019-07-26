




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const NEW_SOURCE_IGNORED_URLS = ["debugger eval code", "self-hosted", "XStringBundle"];
const NEW_SOURCE_DISPLAY_DELAY = 200; 
const FETCH_SOURCE_RESPONSE_DELAY = 200; 
const FETCH_EVENT_LISTENERS_DELAY = 200; 
const FRAME_STEP_CLEAR_DELAY = 100; 
const CALL_STACK_PAGE_SIZE = 25; 


const EVENTS = {
  
  EDITOR_LOADED: "Debugger:EditorLoaded",
  EDITOR_UNLOADED: "Debugger:EditorUnoaded",

  
  NEW_SOURCE: "Debugger:NewSource",
  SOURCES_ADDED: "Debugger:SourcesAdded",

  
  SOURCE_SHOWN: "Debugger:EditorSourceShown",
  SOURCE_ERROR_SHOWN: "Debugger:EditorSourceErrorShown",

  
  
  FETCHED_SCOPES: "Debugger:FetchedScopes",
  FETCHED_VARIABLES: "Debugger:FetchedVariables",
  FETCHED_PROPERTIES: "Debugger:FetchedProperties",
  FETCHED_BUBBLE_PROPERTIES: "Debugger:FetchedBubbleProperties",
  FETCHED_WATCH_EXPRESSIONS: "Debugger:FetchedWatchExpressions",

  
  BREAKPOINT_ADDED: "Debugger:BreakpointAdded",
  BREAKPOINT_REMOVED: "Debugger:BreakpointRemoved",

  
  BREAKPOINT_SHOWN: "Debugger:BreakpointShown",
  BREAKPOINT_HIDDEN: "Debugger:BreakpointHidden",

  
  CONDITIONAL_BREAKPOINT_POPUP_SHOWING: "Debugger:ConditionalBreakpointPopupShowing",
  CONDITIONAL_BREAKPOINT_POPUP_HIDING: "Debugger:ConditionalBreakpointPopupHiding",

  
  EVENT_LISTENERS_FETCHED: "Debugger:EventListenersFetched",
  EVENT_BREAKPOINTS_UPDATED: "Debugger:EventBreakpointsUpdated",

  
  FILE_SEARCH_MATCH_FOUND: "Debugger:FileSearch:MatchFound",
  FILE_SEARCH_MATCH_NOT_FOUND: "Debugger:FileSearch:MatchNotFound",

  
  FUNCTION_SEARCH_MATCH_FOUND: "Debugger:FunctionSearch:MatchFound",
  FUNCTION_SEARCH_MATCH_NOT_FOUND: "Debugger:FunctionSearch:MatchNotFound",

  
  GLOBAL_SEARCH_MATCH_FOUND: "Debugger:GlobalSearch:MatchFound",
  GLOBAL_SEARCH_MATCH_NOT_FOUND: "Debugger:GlobalSearch:MatchNotFound",

  
  AFTER_FRAMES_CLEARED: "Debugger:AfterFramesCleared",

  
  OPTIONS_POPUP_SHOWING: "Debugger:OptionsPopupShowing",
  OPTIONS_POPUP_HIDDEN: "Debugger:OptionsPopupHidden",

  
  LAYOUT_CHANGED: "Debugger:LayoutChanged"
};


const FRAME_TYPE = {
  NORMAL: 0,
  CONDITIONAL_BREAKPOINT_EVAL: 1,
  WATCH_EXPRESSIONS_EVAL: 2,
  PUBLIC_CLIENT_EVAL: 3
};

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");
Cu.import("resource:///modules/devtools/SimpleListWidget.jsm");
Cu.import("resource:///modules/devtools/BreadcrumbsWidget.jsm");
Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/VariablesViewController.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const promise = require("sdk/core/promise");
const Editor = require("devtools/sourceeditor/editor");
const DebuggerEditor = require("devtools/sourceeditor/debugger.js");
const {Tooltip} = require("devtools/shared/widgets/Tooltip");
const FastListWidget = require("devtools/shared/widgets/FastListWidget");

XPCOMUtils.defineLazyModuleGetter(this, "Parser",
  "resource:///modules/devtools/Parser.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "devtools",
  "resource://gre/modules/devtools/Loader.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DevToolsUtils",
  "resource://gre/modules/devtools/DevToolsUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ShortcutUtils",
  "resource://gre/modules/ShortcutUtils.jsm");

Object.defineProperty(this, "NetworkHelper", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/network-helper");
  },
  configurable: true,
  enumerable: true
});




let DebuggerController = {
  


  initialize: function() {
    dumpn("Initializing the DebuggerController");

    this.startupDebugger = this.startupDebugger.bind(this);
    this.shutdownDebugger = this.shutdownDebugger.bind(this);
    this._onTabNavigated = this._onTabNavigated.bind(this);
    this._onTabDetached = this._onTabDetached.bind(this);

    
    
    if (window._isChromeDebugger) {
      window.addEventListener("DOMContentLoaded", this.startupDebugger, true);
      window.addEventListener("unload", this.shutdownDebugger, true);
    }
  },

  





  startupDebugger: function() {
    if (this._startup) {
      return this._startup;
    }

    
    
    if (window._isChromeDebugger) {
      window.removeEventListener("DOMContentLoaded", this.startupDebugger, true);
    }

    return this._startup = DebuggerView.initialize().then(() => {
      
      if (window._isChromeDebugger) {
        return this.connect();
      } else {
        return promise.resolve(null); 
      }
    });
  },

  





  shutdownDebugger: function() {
    if (this._shutdown) {
      return this._shutdown;
    }

    
    
    if (window._isChromeDebugger) {
      window.removeEventListener("unload", this.shutdownDebugger, true);
    }

    return this._shutdown = DebuggerView.destroy().then(() => {
      DebuggerView.destroy();
      this.SourceScripts.disconnect();
      this.StackFrames.disconnect();
      this.ThreadState.disconnect();
      this.Tracer.disconnect();
      this.disconnect();

      
      if (window._isChromeDebugger) {
        return this._quitApp();
      } else {
        return promise.resolve(null); 
      }
    });
  },

  









  connect: function() {
    if (this._connection) {
      return this._connection;
    }

    let startedDebugging = promise.defer();
    this._connection = startedDebugging.promise;

    if (!window._isChromeDebugger) {
      let target = this._target;
      let { client, form: { chromeDebugger, traceActor }, threadActor } = target;
      target.on("close", this._onTabDetached);
      target.on("navigate", this._onTabNavigated);
      target.on("will-navigate", this._onTabNavigated);
      this.client = client;

      if (target.chrome) {
        this._startChromeDebugging(chromeDebugger, startedDebugging.resolve);
      } else {
        this._startDebuggingTab(startedDebugging.resolve);
        const startedTracing = promise.defer();
        this._startTracingTab(traceActor, startedTracing.resolve);

        return promise.all([startedDebugging.promise, startedTracing.promise]);
      }

      return startedDebugging.promise;
    }

    
    let transport = debuggerSocketConnect(
      Prefs.chromeDebuggingHost, Prefs.chromeDebuggingPort);

    let client = this.client = new DebuggerClient(transport);
    client.addListener("tabNavigated", this._onTabNavigated);
    client.addListener("tabDetached", this._onTabDetached);
    client.connect(() => {
      client.listTabs(aResponse => {
        this._startChromeDebugging(aResponse.chromeDebugger, startedDebugging.resolve);
      });
    });

    return startedDebugging.promise;
  },

  


  disconnect: function() {
    
    if (!this.client) {
      return;
    }

    
    
    
    if (window._isChromeDebugger) {
      this.client.removeListener("tabNavigated", this._onTabNavigated);
      this.client.removeListener("tabDetached", this._onTabDetached);
      this.client.close();
    }

    this._connection = null;
    this.client = null;
    this.activeThread = null;
  },

  







  _onTabNavigated: function(aType, aPacket) {
    switch (aType) {
      case "will-navigate": {
        
        DebuggerView.handleTabNavigation();

        
        
        
        DebuggerController.SourceScripts.clearCache();
        DebuggerController.Parser.clearCache();
        SourceUtils.clearCache();

        
        clearNamedTimeout("new-source");
        clearNamedTimeout("event-breakpoints-update");
        clearNamedTimeout("event-listeners-fetch");
        break;
      }
      case "navigate": {
        this.ThreadState.handleTabNavigation();
        this.StackFrames.handleTabNavigation();
        this.SourceScripts.handleTabNavigation();
        break;
      }
    }
  },

  


  _onTabDetached: function() {
    this.shutdownDebugger();
  },

  


  _ensureResumptionOrder: function(aResponse) {
    if (aResponse.error == "wrongOrder") {
      DebuggerView.Toolbar.showResumeWarning(aResponse.lastPausedUrl);
    }
  },

  





  _startDebuggingTab: function(aCallback) {
    this._target.activeTab.attachThread({
      useSourceMaps: Prefs.sourceMapsEnabled
    }, (aResponse, aThreadClient) => {
      if (!aThreadClient) {
        Cu.reportError("Couldn't attach to thread: " + aResponse.error);
        return;
      }
      this.activeThread = aThreadClient;

      this.ThreadState.connect();
      this.StackFrames.connect();
      this.SourceScripts.connect();
      if (aThreadClient.paused) {
        aThreadClient.resume(this._ensureResumptionOrder);
      }

      if (aCallback) {
        aCallback();
      }
    });
  },

  







  _startChromeDebugging: function(aChromeDebugger, aCallback) {
    this.client.attachThread(aChromeDebugger, (aResponse, aThreadClient) => {
      if (!aThreadClient) {
        Cu.reportError("Couldn't attach to thread: " + aResponse.error);
        return;
      }
      this.activeThread = aThreadClient;

      this.ThreadState.connect();
      this.StackFrames.connect();
      this.SourceScripts.connect();
      if (aThreadClient.paused) {
        aThreadClient.resume(this._ensureResumptionOrder);
      }

      if (aCallback) {
        aCallback();
      }
    }, { useSourceMaps: Prefs.sourceMapsEnabled });
  },

  







  _startTracingTab: function(aTraceActor, aCallback) {
    this.client.attachTracer(aTraceActor, (response, traceClient) => {
      if (!traceClient) {
        DevToolsUtils.reportError(new Error("Failed to attach to tracing actor."));
        return;
      }

      this.traceClient = traceClient;
      this.Tracer.connect();

      if (aCallback) {
        aCallback();
      }
    });
  },

  



  reconfigureThread: function(aUseSourceMaps) {
    this.activeThread.reconfigure({ useSourceMaps: aUseSourceMaps }, aResponse => {
      if (aResponse.error) {
        let msg = "Couldn't reconfigure thread: " + aResponse.message;
        Cu.reportError(msg);
        dumpn(msg);
        return;
      }

      
      DebuggerView.handleTabNavigation();
      this.SourceScripts.handleTabNavigation();

      
      if (this.activeThread.paused) {
        this.activeThread._clearFrames();
        this.activeThread.fillFrames(CALL_STACK_PAGE_SIZE);
      }
    });
  },

  





  _quitApp: function() {
    let deferred = promise.defer();

    
    
    Services.tm.currentThread.dispatch({ run: () => {
      let quit = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(quit, "quit-application-requested", null);

      
      if (quit.data) {
        deferred.reject(quit.data);
      } else {
        deferred.resolve(quit.data);
        Services.startup.quit(Ci.nsIAppStartup.eForceQuit);
      }
    }}, 0);

    return deferred.promise;
  },

  _startup: null,
  _shutdown: null,
  _connection: null,
  client: null,
  activeThread: null
};





function ThreadState() {
  this._update = this._update.bind(this);
}

ThreadState.prototype = {
  get activeThread() DebuggerController.activeThread,

  


  connect: function() {
    dumpn("ThreadState is connecting...");
    this.activeThread.addListener("paused", this._update);
    this.activeThread.addListener("resumed", this._update);
    this.activeThread.pauseOnExceptions(Prefs.pauseOnExceptions,
                                        Prefs.ignoreCaughtExceptions);
    this.handleTabNavigation();
  },

  


  disconnect: function() {
    if (!this.activeThread) {
      return;
    }
    dumpn("ThreadState is disconnecting...");
    this.activeThread.removeListener("paused", this._update);
    this.activeThread.removeListener("resumed", this._update);
  },

  


  handleTabNavigation: function() {
    if (!this.activeThread) {
      return;
    }
    dumpn("Handling tab navigation in the ThreadState");
    this._update();
  },

  


  _update: function(aEvent) {
    DebuggerView.Toolbar.toggleResumeButtonState(this.activeThread.state);

    if (gTarget && (aEvent == "paused" || aEvent == "resumed")) {
      gTarget.emit("thread-" + aEvent);
    }
  }
};





function StackFrames() {
  this._onPaused = this._onPaused.bind(this);
  this._onResumed = this._onResumed.bind(this);
  this._onFrames = this._onFrames.bind(this);
  this._onFramesCleared = this._onFramesCleared.bind(this);
  this._onBlackBoxChange = this._onBlackBoxChange.bind(this);
  this._onPrettyPrintChange = this._onPrettyPrintChange.bind(this);
  this._afterFramesCleared = this._afterFramesCleared.bind(this);
  this.evaluate = this.evaluate.bind(this);
}

StackFrames.prototype = {
  get activeThread() DebuggerController.activeThread,
  currentFrameDepth: -1,
  _currentFrameDescription: FRAME_TYPE.NORMAL,
  _syncedWatchExpressions: null,
  _currentWatchExpressions: null,
  _currentBreakpointLocation: null,
  _currentEvaluation: null,
  _currentException: null,
  _currentReturnedValue: null,

  


  connect: function() {
    dumpn("StackFrames is connecting...");
    this.activeThread.addListener("paused", this._onPaused);
    this.activeThread.addListener("resumed", this._onResumed);
    this.activeThread.addListener("framesadded", this._onFrames);
    this.activeThread.addListener("framescleared", this._onFramesCleared);
    this.activeThread.addListener("blackboxchange", this._onBlackBoxChange);
    this.activeThread.addListener("prettyprintchange", this._onPrettyPrintChange);
    this.handleTabNavigation();
  },

  


  disconnect: function() {
    if (!this.activeThread) {
      return;
    }
    dumpn("StackFrames is disconnecting...");
    this.activeThread.removeListener("paused", this._onPaused);
    this.activeThread.removeListener("resumed", this._onResumed);
    this.activeThread.removeListener("framesadded", this._onFrames);
    this.activeThread.removeListener("framescleared", this._onFramesCleared);
    this.activeThread.removeListener("blackboxchange", this._onBlackBoxChange);
    this.activeThread.removeListener("prettyprintchange", this._onPrettyPrintChange);
    clearNamedTimeout("frames-cleared");
  },

  


  handleTabNavigation: function() {
    dumpn("Handling tab navigation in the StackFrames");
    
  },

  







  _onPaused: function(aEvent, aPacket) {
    switch (aPacket.why.type) {
      
      case "breakpoint":
        this._currentBreakpointLocation = aPacket.frame.where;
        break;
      
      case "clientEvaluated":
        this._currentEvaluation = aPacket.why.frameFinished;
        break;
      
      case "exception":
        this._currentException = aPacket.why.exception;
        break;
      
      
      case "resumeLimit":
        if (!aPacket.why.frameFinished) {
          break;
        } else if (aPacket.why.frameFinished.throw) {
          this._currentException = aPacket.why.frameFinished.throw;
        } else if (aPacket.why.frameFinished.return) {
          this._currentReturnedValue = aPacket.why.frameFinished.return;
        }
        break;
    }

    this.activeThread.fillFrames(CALL_STACK_PAGE_SIZE);
    DebuggerView.editor.focus();
  },

  


  _onResumed: function() {
    
    if (this._currentFrameDescription != FRAME_TYPE.WATCH_EXPRESSIONS_EVAL) {
      this._currentWatchExpressions = this._syncedWatchExpressions;
    }
  },

  


  _onFrames: function() {
    
    if (!this.activeThread || !this.activeThread.cachedFrames.length) {
      return;
    }

    let waitForNextPause = false;
    let breakLocation = this._currentBreakpointLocation;
    let watchExpressions = this._currentWatchExpressions;

    
    
    
    
    if (breakLocation) {
      
      let breakpointPromise = DebuggerController.Breakpoints._getAdded(breakLocation);
      if (breakpointPromise) {
        breakpointPromise.then(({ conditionalExpression: e }) => { if (e) {
          
          
          
          this.evaluate(e, { depth: 0, meta: FRAME_TYPE.CONDITIONAL_BREAKPOINT_EVAL });
          waitForNextPause = true;
        }});
      }
    }
    
    
    if (waitForNextPause) {
      return;
    }
    if (this._currentFrameDescription == FRAME_TYPE.CONDITIONAL_BREAKPOINT_EVAL) {
      this._currentFrameDescription = FRAME_TYPE.NORMAL;
      
      
      if (VariablesView.isFalsy({ value: this._currentEvaluation.return })) {
        this.activeThread.resume(DebuggerController._ensureResumptionOrder);
        return;
      }
    }

    
    
    
    if (watchExpressions) {
      
      
      this.evaluate(watchExpressions, { depth: 0, meta: FRAME_TYPE.WATCH_EXPRESSIONS_EVAL });
      waitForNextPause = true;
    }
    
    
    if (waitForNextPause) {
      return;
    }
    if (this._currentFrameDescription == FRAME_TYPE.WATCH_EXPRESSIONS_EVAL) {
      this._currentFrameDescription = FRAME_TYPE.NORMAL;
      
      
      
      if (this._currentEvaluation.throw) {
        DebuggerView.WatchExpressions.removeAt(0);
        DebuggerController.StackFrames.syncWatchExpressions();
        return;
      }
    }

    
    DebuggerView.showInstrumentsPane();
    this._refillFrames();

    
    if (this._currentFrameDescription != FRAME_TYPE.NORMAL) {
      this._currentFrameDescription = FRAME_TYPE.NORMAL;
    }
  },

  



  _refillFrames: function() {
    
    DebuggerView.StackFrames.empty();

    for (let frame of this.activeThread.cachedFrames) {
      let { depth, where: { url, line }, source } = frame;
      let isBlackBoxed = source ? this.activeThread.source(source).isBlackBoxed : false;
      let location = NetworkHelper.convertToUnicode(unescape(url));
      let title = StackFrameUtils.getFrameTitle(frame);
      DebuggerView.StackFrames.addFrame(title, location, line, depth, isBlackBoxed);
    }

    DebuggerView.StackFrames.selectedDepth = Math.max(this.currentFrameDepth, 0);
    DebuggerView.StackFrames.dirty = this.activeThread.moreFrames;
  },

  


  _onFramesCleared: function() {
    switch (this._currentFrameDescription) {
      case FRAME_TYPE.NORMAL:
        this._currentEvaluation = null;
        this._currentException = null;
        this._currentReturnedValue = null;
        break;
      case FRAME_TYPE.CONDITIONAL_BREAKPOINT_EVAL:
        this._currentBreakpointLocation = null;
        break;
      case FRAME_TYPE.WATCH_EXPRESSIONS_EVAL:
        this._currentWatchExpressions = null;
        break;
    }

    
    
    
    
    setNamedTimeout("frames-cleared", FRAME_STEP_CLEAR_DELAY, this._afterFramesCleared);
  },

  


  _onBlackBoxChange: function() {
    if (this.activeThread.state == "paused") {
      
      this.currentFrameDepth = NaN;
      this._refillFrames();
    }
  },

  


  _onPrettyPrintChange: function() {
    if (this.activeThread.state == "paused") {
      this.activeThread.fillFrames(CALL_STACK_PAGE_SIZE);
    }
  },

  


  _afterFramesCleared: function() {
    
    if (this.activeThread.cachedFrames.length) {
      return;
    }
    DebuggerView.editor.clearDebugLocation();
    DebuggerView.StackFrames.empty();
    DebuggerView.Sources.unhighlightBreakpoint();
    DebuggerView.WatchExpressions.toggleContents(true);
    DebuggerView.Variables.empty(0);

    window.emit(EVENTS.AFTER_FRAMES_CLEARED);
  },

  






  selectFrame: function(aDepth) {
    
    let frame = this.activeThread.cachedFrames[this.currentFrameDepth = aDepth];
    if (!frame) {
      return;
    }

    
    let { environment, where } = frame;
    if (!environment) {
      return;
    }

    
    
    
    let isClientEval = this._currentFrameDescription == FRAME_TYPE.PUBLIC_CLIENT_EVAL;
    let isPopupShown = DebuggerView.VariableBubble.contentsShown();
    if (!isClientEval && !isPopupShown) {
      
      DebuggerView.setEditorLocation(where.url, where.line);
      
      DebuggerView.Sources.highlightBreakpoint(where, { noEditorUpdate: true });
    }

    
    DebuggerView.WatchExpressions.toggleContents(false);

    
    
    DebuggerView.Variables.empty();

    
    
    if (this._syncedWatchExpressions && aDepth == 0) {
      let label = L10N.getStr("watchExpressionsScopeLabel");
      let scope = DebuggerView.Variables.addScope(label);

      
      scope.descriptorTooltip = false;
      scope.contextMenuId = "debuggerWatchExpressionsContextMenu";
      scope.separatorStr = L10N.getStr("watchExpressionsSeparatorLabel");
      scope.switch = DebuggerView.WatchExpressions.switchExpression;
      scope.delete = DebuggerView.WatchExpressions.deleteExpression;

      
      this._fetchWatchExpressions(scope, this._currentEvaluation.return);

      
      scope.expand();
    }

    do {
      
      
      let label = StackFrameUtils.getScopeLabel(environment);
      let scope = DebuggerView.Variables.addScope(label);
      let innermost = environment == frame.environment;

      
      if (innermost) {
        this._insertScopeFrameReferences(scope, frame);
      }

      
      
      DebuggerView.Variables.controller.addExpander(scope, environment);

      
      
      
      if (innermost) {
        scope.expand();
      }
    } while ((environment = environment.parent));

    
    window.emit(EVENTS.FETCHED_SCOPES);
  },

  


  addMoreFrames: function() {
    this.activeThread.fillFrames(
      this.activeThread.cachedFrames.length + CALL_STACK_PAGE_SIZE);
  },

  













  evaluate: function(aExpression, aOptions = {}) {
    let depth = "depth" in aOptions ? aOptions.depth : this.currentFrameDepth;
    let frame = this.activeThread.cachedFrames[depth];
    if (frame == null) {
      return promise.reject(new Error("No stack frame available."));
    }

    let deferred = promise.defer();

    this.activeThread.addOneTimeListener("paused", (aEvent, aPacket) => {
      let { type, frameFinished } = aPacket.why;
      if (type == "clientEvaluated") {
        if (!("terminated" in frameFinished)) {
          deferred.resolve(frameFinished);
        } else {
          deferred.reject(new Error("The execution was abruptly terminated."));
        }
      } else {
        deferred.reject(new Error("Active thread paused unexpectedly."));
      }
    });

    let meta = "meta" in aOptions ? aOptions.meta : FRAME_TYPE.PUBLIC_CLIENT_EVAL;
    this._currentFrameDescription = meta;
    this.activeThread.eval(frame.actor, aExpression);

    return deferred.promise;
  },

  







  _insertScopeFrameReferences: function(aScope, aFrame) {
    
    if (this._currentException) {
      let excRef = aScope.addItem("<exception>", { value: this._currentException });
      DebuggerView.Variables.controller.addExpander(excRef, this._currentException);
    }
    
    if (this._currentReturnedValue) {
      let retRef = aScope.addItem("<return>", { value: this._currentReturnedValue });
      DebuggerView.Variables.controller.addExpander(retRef, this._currentReturnedValue);
    }
    
    if (aFrame.this) {
      let thisRef = aScope.addItem("this", { value: aFrame.this });
      DebuggerView.Variables.controller.addExpander(thisRef, aFrame.this);
    }
  },

  







  _fetchWatchExpressions: function(aScope, aExp) {
    
    if (aScope._fetched) {
      return;
    }
    aScope._fetched = true;

    
    this.activeThread.pauseGrip(aExp).getPrototypeAndProperties(aResponse => {
      let ownProperties = aResponse.ownProperties;
      let totalExpressions = DebuggerView.WatchExpressions.itemCount;

      for (let i = 0; i < totalExpressions; i++) {
        let name = DebuggerView.WatchExpressions.getString(i);
        let expVal = ownProperties[i].value;
        let expRef = aScope.addItem(name, ownProperties[i]);
        DebuggerView.Variables.controller.addExpander(expRef, expVal);

        
        
        expRef.switch = null;
        expRef.delete = null;
        expRef.descriptorTooltip = true;
        expRef.separatorStr = L10N.getStr("variablesSeparatorLabel");
      }

      
      window.emit(EVENTS.FETCHED_WATCH_EXPRESSIONS);
    });
  },

  



  syncWatchExpressions: function() {
    let list = DebuggerView.WatchExpressions.getAllStrings();

    
    
    
    
    let sanitizedExpressions = list.map(aString => {
      
      try {
        Parser.reflectionAPI.parse(aString);
        return aString; 
      } catch (e) {
        return "\"" + e.name + ": " + e.message + "\""; 
      }
    });

    if (sanitizedExpressions.length) {
      this._syncedWatchExpressions =
        this._currentWatchExpressions =
          "[" +
            sanitizedExpressions.map(aString =>
              "eval(\"" +
                "try {" +
                  
                  
                  
                  aString.replace(/"/g, "\\$&") + "\" + " + "'\\n'" + " + \"" +
                "} catch (e) {" +
                  "e.name + ': ' + e.message;" + 
                "}" +
              "\")"
            ).join(",") +
          "]";
    } else {
      this._syncedWatchExpressions =
        this._currentWatchExpressions = null;
    }

    this.currentFrameDepth = -1;
    this._onFrames();
  }
};





function SourceScripts() {
  this._onNewGlobal = this._onNewGlobal.bind(this);
  this._onNewSource = this._onNewSource.bind(this);
  this._onSourcesAdded = this._onSourcesAdded.bind(this);
  this._onBlackBoxChange = this._onBlackBoxChange.bind(this);
  this._onPrettyPrintChange = this._onPrettyPrintChange.bind(this);
}

SourceScripts.prototype = {
  get activeThread() DebuggerController.activeThread,
  get debuggerClient() DebuggerController.client,
  _cache: new Map(),

  


  connect: function() {
    dumpn("SourceScripts is connecting...");
    this.debuggerClient.addListener("newGlobal", this._onNewGlobal);
    this.debuggerClient.addListener("newSource", this._onNewSource);
    this.activeThread.addListener("blackboxchange", this._onBlackBoxChange);
    this.activeThread.addListener("prettyprintchange", this._onPrettyPrintChange);
    this.handleTabNavigation();
  },

  


  disconnect: function() {
    if (!this.activeThread) {
      return;
    }
    dumpn("SourceScripts is disconnecting...");
    this.debuggerClient.removeListener("newGlobal", this._onNewGlobal);
    this.debuggerClient.removeListener("newSource", this._onNewSource);
    this.activeThread.removeListener("blackboxchange", this._onBlackBoxChange);
    this.activeThread.addListener("prettyprintchange", this._onPrettyPrintChange);
  },

  


  clearCache: function() {
    this._cache.clear();
  },

  


  handleTabNavigation: function() {
    if (!this.activeThread) {
      return;
    }
    dumpn("Handling tab navigation in the SourceScripts");

    
    
    this.activeThread.getSources(this._onSourcesAdded);
  },

  


  _onNewGlobal: function(aNotification, aPacket) {
    
    
  },

  


  _onNewSource: function(aNotification, aPacket) {
    
    if (NEW_SOURCE_IGNORED_URLS.indexOf(aPacket.source.url) != -1) {
      return;
    }

    
    DebuggerView.Sources.addSource(aPacket.source, { staged: false });

    
    let preferredValue = DebuggerView.Sources.preferredValue;
    if (aPacket.source.url == preferredValue) {
      DebuggerView.Sources.selectedValue = preferredValue;
    }
    
    else {
      setNamedTimeout("new-source", NEW_SOURCE_DISPLAY_DELAY, () => {
        
        
        if (!DebuggerView.Sources.selectedValue) {
          DebuggerView.Sources.selectedIndex = 0;
        }
      });
    }

    
    
    DebuggerController.Breakpoints.updateEditorBreakpoints();
    DebuggerController.Breakpoints.updatePaneBreakpoints();

    
    if (DebuggerView.instrumentsPaneTab == "events-tab") {
      DebuggerController.Breakpoints.DOM.scheduleEventListenersFetch();
    }

    
    window.emit(EVENTS.NEW_SOURCE);
  },

  


  _onSourcesAdded: function(aResponse) {
    if (aResponse.error) {
      let msg = "Error getting sources: " + aResponse.message;
      Cu.reportError(msg);
      dumpn(msg);
      return;
    }

    
    for (let source of aResponse.sources) {
      
      if (NEW_SOURCE_IGNORED_URLS.indexOf(source.url) == -1) {
        DebuggerView.Sources.addSource(source, { staged: true });
      }
    }

    
    DebuggerView.Sources.commit({ sorted: true });

    
    let preferredValue = DebuggerView.Sources.preferredValue;
    if (DebuggerView.Sources.containsValue(preferredValue)) {
      DebuggerView.Sources.selectedValue = preferredValue;
    }
    
    else if (!DebuggerView.Sources.selectedValue) {
      DebuggerView.Sources.selectedIndex = 0;
    }

    
    
    DebuggerController.Breakpoints.updateEditorBreakpoints();
    DebuggerController.Breakpoints.updatePaneBreakpoints();

    
    window.emit(EVENTS.SOURCES_ADDED);
  },

  


  _onBlackBoxChange: function (aEvent, { url, isBlackBoxed }) {
    const item = DebuggerView.Sources.getItemByValue(url);
    if (item) {
      if (isBlackBoxed) {
        item.target.classList.add("black-boxed");
      } else {
        item.target.classList.remove("black-boxed");
      }
    }
    DebuggerView.Sources.updateToolbarButtonsState();
    DebuggerView.maybeShowBlackBoxMessage();
  },

  










  setBlackBoxing: function(aSource, aBlackBoxFlag) {
    const sourceClient = this.activeThread.source(aSource);
    const deferred = promise.defer();

    sourceClient[aBlackBoxFlag ? "blackBox" : "unblackBox"](aPacket => {
      const { error, message } = aPacket;
      if (error) {
        let msg = "Couldn't toggle black boxing for " + aSource.url + ": " + message;
        dumpn(msg);
        Cu.reportError(msg);
        deferred.reject([aSource, msg]);
      } else {
        deferred.resolve([aSource, sourceClient.isBlackBoxed]);
      }
    });

    return deferred.promise;
  },

  










  togglePrettyPrint: function(aSource) {
    
    if (!SourceUtils.isJavaScript(aSource.url, aSource.contentType)) {
      return promise.reject([aSource, "Can't prettify non-javascript files."]);
    }

    const sourceClient = this.activeThread.source(aSource);
    const wantPretty = !sourceClient.isPrettyPrinted;

    
    let textPromise = this._cache.get(aSource.url);
    if (textPromise && textPromise.pretty === wantPretty) {
      return textPromise;
    }

    const deferred = promise.defer();
    deferred.promise.pretty = wantPretty;
    this._cache.set(aSource.url, deferred.promise);

    const afterToggle = ({ error, message, source: text }) => {
      if (error) {
        
        
        this._cache.set(aSource.url, textPromise);
        deferred.reject([aSource, message || error]);
        return;
      }
      deferred.resolve([aSource, text]);
    };

    if (wantPretty) {
      sourceClient.prettyPrint(Prefs.editorTabSize, afterToggle);
    } else {
      sourceClient.disablePrettyPrint(afterToggle);
    }

    return deferred.promise;
  },

  


  _onPrettyPrintChange: function(aEvent, { url }) {
    
    
    DebuggerController.Parser.clearSource(url);
  },

  














  getText: function(aSource, aOnTimeout, aDelay = FETCH_SOURCE_RESPONSE_DELAY) {
    
    let textPromise = this._cache.get(aSource.url);
    if (textPromise) {
      return textPromise;
    }

    let deferred = promise.defer();
    this._cache.set(aSource.url, deferred.promise);

    
    if (aOnTimeout) {
      var fetchTimeout = window.setTimeout(() => aOnTimeout(aSource), aDelay);
    }

    
    this.activeThread.source(aSource).source(({ error, message, source: text }) => {
      if (aOnTimeout) {
        window.clearTimeout(fetchTimeout);
      }
      if (error) {
        deferred.reject([aSource, message || error]);
      } else {
        deferred.resolve([aSource, text]);
      }
    });

    return deferred.promise;
  },

  








  getTextForSources: function(aUrls) {
    let deferred = promise.defer();
    let pending = new Set(aUrls);
    let fetched = [];

    
    
    
    

    
    for (let url of aUrls) {
      let sourceItem = DebuggerView.Sources.getItemByValue(url);
      let sourceForm = sourceItem.attachment.source;
      this.getText(sourceForm, onTimeout).then(onFetch, onError);
    }

    
    function onTimeout(aSource) {
      onError([aSource]);
    }

    
    function onFetch([aSource, aText]) {
      
      if (!pending.has(aSource.url)) {
        return;
      }
      pending.delete(aSource.url);
      fetched.push([aSource.url, aText]);
      maybeFinish();
    }

    
    function onError([aSource, aError]) {
      pending.delete(aSource.url);
      maybeFinish();
    }

    
    function maybeFinish() {
      if (pending.size == 0) {
        
        deferred.resolve(fetched.sort(([aFirst], [aSecond]) => aFirst > aSecond));
      }
    }

    return deferred.promise;
  }
};





function Tracer() {
  this._trace = null;
  this._idCounter = 0;
  this.onTraces = this.onTraces.bind(this);
}

Tracer.prototype = {
  get client() {
    return DebuggerController.client;
  },

  get traceClient() {
    return DebuggerController.traceClient;
  },

  get tracing() {
    return !!this._trace;
  },

  


  connect: function() {
    this._stack = [];
    this.client.addListener("traces", this.onTraces);
  },

  



  disconnect: function() {
    this._stack = null;
    this.client.removeListener("traces", this.onTraces);
  },

  


  startTracing: function(aCallback = () => {}) {
    DebuggerView.Tracer.selectTab();
    if (this.tracing) {
      return;
    }
    this._trace = "dbg.trace" + Math.random();
    this.traceClient.startTrace([
      "name",
      "location",
      "parameterNames",
      "depth",
      "arguments",
      "return",
      "throw",
      "yield"
    ], this._trace, (aResponse) => {
      const { error } = aResponse;
      if (error) {
        DevToolsUtils.reportException(error);
        this._trace = null;
      }

      aCallback(aResponse);
    });
  },

  


  stopTracing: function(aCallback = () => {}) {
    if (!this.tracing) {
      return;
    }
    this.traceClient.stopTrace(this._trace, aResponse => {
      const { error } = aResponse;
      if (error) {
        DevToolsUtils.reportException(error);
      }

      this._trace = null;
      aCallback(aResponse);
    });
  },

  onTraces: function (aEvent, { traces }) {
    const tracesLength = traces.length;
    let tracesToShow;
    if (tracesLength > TracerView.MAX_TRACES) {
      tracesToShow = traces.slice(tracesLength - TracerView.MAX_TRACES,
                                  tracesLength);
      DebuggerView.Tracer.empty();
      this._stack.splice(0, this._stack.length);
    } else {
      tracesToShow = traces;
    }

    for (let t of tracesToShow) {
      if (t.type == "enteredFrame") {
        this._onCall(t);
      } else {
        this._onReturn(t);
      }
    }

    DebuggerView.Tracer.commit();
  },

  


  _onCall: function({ name, location, parameterNames, depth, arguments: args }) {
    const item = {
      name: name,
      location: location,
      id: this._idCounter++
    };

    this._stack.push(item);
    DebuggerView.Tracer.addTrace({
      type: "call",
      name: name,
      location: location,
      depth: depth,
      parameterNames: parameterNames,
      arguments: args,
      frameId: item.id
    });
  },

  


  _onReturn: function(aPacket) {
    if (!this._stack.length) {
      return;
    }

    const { name, id, location } = this._stack.pop();
    DebuggerView.Tracer.addTrace({
      type: aPacket.why,
      name: name,
      location: location,
      depth: aPacket.depth,
      frameId: id,
      returnVal: aPacket.return || aPacket.throw || aPacket.yield
    });
  },

  










  syncGripClient: function(aObject) {
    return {
      get isFrozen() { return aObject.frozen; },
      get isSealed() { return aObject.sealed; },
      get isExtensible() { return aObject.extensible; },

      get ownProperties() { return aObject.ownProperties; },
      get prototype() { return null; },

      getParameterNames: callback => callback(aObject),
      getPrototypeAndProperties: callback => callback(aObject),
      getPrototype: callback => callback(aObject),

      getOwnPropertyNames: (callback) => {
        callback({
          ownPropertyNames: aObject.ownProperties
            ? Object.keys(aObject.ownProperties)
            : []
        });
      },

      getProperty: (property, callback) => {
        callback({
          descriptor: aObject.ownProperties
            ? aObject.ownProperties[property]
            : null
        });
      },

      getDisplayString: callback => callback("[object " + aObject.class + "]"),

      getScope: callback => callback({
        error: "scopeNotAvailable",
        message: "Cannot get scopes for traced objects"
      })
    };
  },

  







  WrappedObject: function(aObject) {
    this.object = aObject;
  }
};




function EventListeners() {
  this._onEventListeners = this._onEventListeners.bind(this);
}

EventListeners.prototype = {
  



  activeEventNames: [],

  




  scheduleEventBreakpointsUpdate: function() {
    
    
    setNamedTimeout("event-breakpoints-update", 0, () => {
      this.activeEventNames = DebuggerView.EventListeners.getCheckedEvents();
      gThreadClient.pauseOnDOMEvents(this.activeEventNames);

      
      window.emit(EVENTS.EVENT_BREAKPOINTS_UPDATED);
    });
  },

  


  scheduleEventListenersFetch: function() {
    let getListeners = aCallback => gThreadClient.eventListeners(aResponse => {
      if (aResponse.error) {
        let msg = "Error getting event listeners: " + aResponse.message;
        DevToolsUtils.reportException("scheduleEventListenersFetch", msg);
        return;
      }

      let outstandingListenersDefinitionSite = aResponse.listeners.map(aListener => {
        const deferred = promise.defer();

        gThreadClient.pauseGrip(aListener.function).getDefinitionSite(aResponse => {
          if (aResponse.error) {
            const msg = "Error getting function definition site: " + aResponse.message;
            DevToolsUtils.reportException("scheduleEventListenersFetch", msg);
            deferred.reject(msg);
            return;
          }

          aListener.function.url = aResponse.url;
          deferred.resolve(aListener);
        });

        return deferred.promise;
      });

      promise.all(outstandingListenersDefinitionSite).then(aListeners => {
        this._onEventListeners(aListeners);

        
        
        window.emit(EVENTS.EVENT_LISTENERS_FETCHED);
        aCallback && aCallback();
      });
    });

    
    
    setNamedTimeout("event-listeners-fetch", FETCH_EVENT_LISTENERS_DELAY, () => {
      if (gThreadClient.state != "paused") {
        gThreadClient.interrupt(() => getListeners(() => gThreadClient.resume()));
      } else {
        getListeners();
      }
    });
  },

  


  _onEventListeners: function(aListeners) {
    
    for (let listener of aListeners) {
      DebuggerView.EventListeners.addListener(listener, { staged: true });
    }

    
    DebuggerView.EventListeners.commit();
  }
};




function Breakpoints() {
  this._onEditorBreakpointAdd = this._onEditorBreakpointAdd.bind(this);
  this._onEditorBreakpointRemove = this._onEditorBreakpointRemove.bind(this);
  this.addBreakpoint = this.addBreakpoint.bind(this);
  this.removeBreakpoint = this.removeBreakpoint.bind(this);
}

Breakpoints.prototype = {
  



  _added: new Map(),
  _removing: new Map(),
  _disabled: new Map(),

  





  initialize: function() {
    DebuggerView.editor.on("breakpointAdded", this._onEditorBreakpointAdd);
    DebuggerView.editor.on("breakpointRemoved", this._onEditorBreakpointRemove);

    
    return promise.resolve(null);
  },

  





  destroy: function() {
    DebuggerView.editor.off("breakpointAdded", this._onEditorBreakpointAdd);
    DebuggerView.editor.off("breakpointRemoved", this._onEditorBreakpointRemove);

    return this.removeAllBreakpoints();
  },

  





  _onEditorBreakpointAdd: function(_, aLine) {
    let url = DebuggerView.Sources.selectedValue;
    let location = { url: url, line: aLine + 1 };

    
    
    this.addBreakpoint(location, { noEditorUpdate: true }).then(aBreakpointClient => {
      
      
      
      if (aBreakpointClient.requestedLocation) {
        DebuggerView.editor.removeBreakpoint(aBreakpointClient.requestedLocation.line - 1);
        DebuggerView.editor.addBreakpoint(aBreakpointClient.location.line - 1);
      }
      
      window.emit(EVENTS.BREAKPOINT_SHOWN);
    });
  },

  





  _onEditorBreakpointRemove: function(_, aLine) {
    let url = DebuggerView.Sources.selectedValue;
    let location = { url: url, line: aLine + 1 };

    
    
    this.removeBreakpoint(location, { noEditorUpdate: true }).then(() => {
      
      window.emit(EVENTS.BREAKPOINT_HIDDEN);
    });
  },

  





  updateEditorBreakpoints: function() {
    for (let breakpointPromise of this._addedOrDisabled) {
      breakpointPromise.then(aBreakpointClient => {
        let currentSourceUrl = DebuggerView.Sources.selectedValue;
        let breakpointUrl = aBreakpointClient.location.url;

        
        if (currentSourceUrl == breakpointUrl) {
          this._showBreakpoint(aBreakpointClient, { noPaneUpdate: true });
        }
      });
    }
  },

  





  updatePaneBreakpoints: function() {
    for (let breakpointPromise of this._addedOrDisabled) {
      breakpointPromise.then(aBreakpointClient => {
        let container = DebuggerView.Sources;
        let breakpointUrl = aBreakpointClient.location.url;

        
        if (container.containsValue(breakpointUrl)) {
          this._showBreakpoint(aBreakpointClient, { noEditorUpdate: true });
        }
      });
    }
  },

  
















  addBreakpoint: function(aLocation, aOptions = {}) {
    
    if (!aLocation) {
      return promise.reject(new Error("Invalid breakpoint location."));
    }

    
    
    let addedPromise = this._getAdded(aLocation);
    if (addedPromise) {
      return addedPromise;
    }

    
    
    let removingPromise = this._getRemoving(aLocation);
    if (removingPromise) {
      return removingPromise.then(() => this.addBreakpoint(aLocation, aOptions));
    }

    let deferred = promise.defer();

    
    let identifier = this.getIdentifier(aLocation);
    this._added.set(identifier, deferred.promise);

    
    gThreadClient.setBreakpoint(aLocation, (aResponse, aBreakpointClient) => {
      
      
      if (aResponse.actualLocation) {
        
        let oldIdentifier = identifier;
        let newIdentifier = identifier = this.getIdentifier(aResponse.actualLocation);
        this._added.delete(oldIdentifier);
        this._added.set(newIdentifier, deferred.promise);

        
        
        aBreakpointClient.requestedLocation = aLocation;
        aBreakpointClient.location = aResponse.actualLocation;
      }

      
      
      
      let disabledPromise = this._disabled.get(identifier);
      if (disabledPromise) {
        disabledPromise.then(({ conditionalExpression: previousValue }) => {
          
          if (previousValue) {
            aBreakpointClient.conditionalExpression = previousValue;
          }
        });
        this._disabled.delete(identifier);
      }

      
      
      
      
      let line = aBreakpointClient.location.line - 1;
      aBreakpointClient.text = DebuggerView.editor.getText(line).trim();

      
      this._showBreakpoint(aBreakpointClient, aOptions);

      
      window.emit(EVENTS.BREAKPOINT_ADDED, aBreakpointClient);
      deferred.resolve(aBreakpointClient);
    });

    return deferred.promise;
  },

  










  removeBreakpoint: function(aLocation, aOptions = {}) {
    
    if (!aLocation) {
      return promise.reject(new Error("Invalid breakpoint location."));
    }

    
    
    let addedPromise = this._getAdded(aLocation);
    if (!addedPromise) {
      return promise.resolve(aLocation);
    }

    
    
    let removingPromise = this._getRemoving(aLocation);
    if (removingPromise) {
      return removingPromise;
    }

    let deferred = promise.defer();

    
    let identifier = this.getIdentifier(aLocation);
    this._removing.set(identifier, deferred.promise);

    
    addedPromise.then(aBreakpointClient => {
      
      aBreakpointClient.remove(aResponse => {
        
        
        if (aResponse.error) {
          deferred.reject(aResponse);
          return void this._removing.delete(identifier);
        }

        
        
        
        
        if (aOptions.rememberDisabled) {
          aBreakpointClient.disabled = true;
          this._disabled.set(identifier, promise.resolve(aBreakpointClient));
        }

        
        this._added.delete(identifier);
        this._removing.delete(identifier);

        
        this._hideBreakpoint(aLocation, aOptions);

        
        window.emit(EVENTS.BREAKPOINT_REMOVED, aLocation);
        deferred.resolve(aLocation);
      });
    });

    return deferred.promise;
  },

  






  removeAllBreakpoints: function() {
    
    let getActiveBreakpoints = (aPromises, aStore = []) => {
      for (let [, breakpointPromise] of aPromises) {
        aStore.push(breakpointPromise);
      }
      return aStore;
    }

    
    let getRemovedBreakpoints = (aClients, aStore = []) => {
      for (let breakpointClient of aClients) {
        aStore.push(this.removeBreakpoint(breakpointClient.location));
      }
      return aStore;
    }

    
    
    
    return promise.all(getActiveBreakpoints(this._added)).then(aBreakpointClients => {
      return promise.all(getRemovedBreakpoints(aBreakpointClients));
    });
  },

  











  _showBreakpoint: function(aBreakpointData, aOptions = {}) {
    let currentSourceUrl = DebuggerView.Sources.selectedValue;
    let location = aBreakpointData.location;

    
    if (!aOptions.noEditorUpdate && !aBreakpointData.disabled) {
      if (location.url == currentSourceUrl) {
        DebuggerView.editor.addBreakpoint(location.line - 1);
      }
    }

    
    if (!aOptions.noPaneUpdate) {
      DebuggerView.Sources.addBreakpoint(aBreakpointData, aOptions);
    }
  },

  







  _hideBreakpoint: function(aLocation, aOptions = {}) {
    let currentSourceUrl = DebuggerView.Sources.selectedValue;

    
    if (!aOptions.noEditorUpdate) {
      if (aLocation.url == currentSourceUrl) {
        DebuggerView.editor.removeBreakpoint(aLocation.line - 1);
      }
    }

    
    if (!aOptions.noPaneUpdate) {
      DebuggerView.Sources.removeBreakpoint(aLocation);
    }
  },

  




  get _addedOrDisabled() {
    for (let [, value] of this._added) yield value;
    for (let [, value] of this._disabled) yield value;
  },

  









  _getAdded: function(aLocation) {
    return this._added.get(this.getIdentifier(aLocation));
  },

  









  _getRemoving: function(aLocation) {
    return this._removing.get(this.getIdentifier(aLocation));
  },

  








  getIdentifier: function(aLocation) {
    return aLocation.url + ":" + aLocation.line;
  }
};




let L10N = new ViewHelpers.L10N(DBG_STRINGS_URI);




let Prefs = new ViewHelpers.Prefs("devtools", {
  chromeDebuggingHost: ["Char", "debugger.chrome-debugging-host"],
  chromeDebuggingPort: ["Int", "debugger.chrome-debugging-port"],
  sourcesWidth: ["Int", "debugger.ui.panes-sources-width"],
  instrumentsWidth: ["Int", "debugger.ui.panes-instruments-width"],
  panesVisibleOnStartup: ["Bool", "debugger.ui.panes-visible-on-startup"],
  variablesSortingEnabled: ["Bool", "debugger.ui.variables-sorting-enabled"],
  variablesOnlyEnumVisible: ["Bool", "debugger.ui.variables-only-enum-visible"],
  variablesSearchboxVisible: ["Bool", "debugger.ui.variables-searchbox-visible"],
  pauseOnExceptions: ["Bool", "debugger.pause-on-exceptions"],
  ignoreCaughtExceptions: ["Bool", "debugger.ignore-caught-exceptions"],
  sourceMapsEnabled: ["Bool", "debugger.source-maps-enabled"],
  prettyPrintEnabled: ["Bool", "debugger.pretty-print-enabled"],
  autoPrettyPrint: ["Bool", "debugger.auto-pretty-print"],
  tracerEnabled: ["Bool", "debugger.tracer"],
  editorTabSize: ["Int", "editor.tabsize"]
});





XPCOMUtils.defineLazyGetter(window, "_isChromeDebugger", function() {
  
  return !(window.frameElement instanceof XULElement);
});




EventEmitter.decorate(this);




DebuggerController.initialize();
DebuggerController.Parser = new Parser();
DebuggerController.ThreadState = new ThreadState();
DebuggerController.StackFrames = new StackFrames();
DebuggerController.SourceScripts = new SourceScripts();
DebuggerController.Breakpoints = new Breakpoints();
DebuggerController.Breakpoints.DOM = new EventListeners();
DebuggerController.Tracer = new Tracer();




Object.defineProperties(window, {
  "gTarget": {
    get: function() DebuggerController._target
  },
  "gHostType": {
    get: function() DebuggerView._hostType
  },
  "gClient": {
    get: function() DebuggerController.client
  },
  "gThreadClient": {
    get: function() DebuggerController.activeThread
  },
  "gCallStackPageSize": {
    get: function() CALL_STACK_PAGE_SIZE
  }
});





function dumpn(str) {
  if (wantLogging) {
    dump("DBG-FRONTEND: " + str + "\n");
  }
}

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");
