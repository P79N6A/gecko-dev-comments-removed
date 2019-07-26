




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const NEW_SOURCE_IGNORED_URLS = ["debugger eval code", "self-hosted", "XStringBundle"];
const NEW_SOURCE_DISPLAY_DELAY = 200; 
const FETCH_SOURCE_RESPONSE_DELAY = 50; 
const FRAME_STEP_CLEAR_DELAY = 100; 
const CALL_STACK_PAGE_SIZE = 25; 

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource:///modules/source-editor.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");
Cu.import("resource:///modules/devtools/BreadcrumbsWidget.jsm");
Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Parser",
  "resource:///modules/devtools/Parser.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetworkHelper",
  "resource://gre/modules/devtools/NetworkHelper.jsm");




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
    if (this._isInitialized) {
      return this._startup.promise;
    }
    this._isInitialized = true;
    window.removeEventListener("DOMContentLoaded", this.startupDebugger, true);

    let deferred = this._startup = Promise.defer();

    DebuggerView.initialize(() => {
      DebuggerView._isInitialized = true;

      
      if (window._isChromeDebugger) {
        this.connect().then(deferred.resolve);
      } else {
        deferred.resolve();
      }
    });

    return deferred.promise;
  },

  





  shutdownDebugger: function() {
    if (this._isDestroyed) {
      return this._shutdown.promise;
    }
    this._isDestroyed = true;
    this._startup = null;
    window.removeEventListener("unload", this.shutdownDebugger, true);

    let deferred = this._shutdown = Promise.defer();

    DebuggerView.destroy(() => {
      DebuggerView._isDestroyed = true;
      this.SourceScripts.disconnect();
      this.StackFrames.disconnect();
      this.ThreadState.disconnect();

      this.disconnect();
      deferred.resolve();

      
      window._isChromeDebugger && this._quitApp();
    });

    return deferred.promise;
  },

  









  connect: function() {
    if (this._connection) {
      return this._connection.promise;
    }

    let deferred = this._connection = Promise.defer();

    if (!window._isChromeDebugger) {
      let target = this._target;
      let { client, form, threadActor } = target;
      target.on("close", this._onTabDetached);
      target.on("navigate", this._onTabNavigated);
      target.on("will-navigate", this._onTabNavigated);

      if (target.chrome) {
        this._startChromeDebugging(client, form.chromeDebugger, deferred.resolve);
      } else {
        this._startDebuggingTab(client, threadActor, deferred.resolve);
      }

      return deferred.promise;
    }

    
    let transport = debuggerSocketConnect(Prefs.chromeDebuggingHost,
                                          Prefs.chromeDebuggingPort);

    let client = new DebuggerClient(transport);
    client.addListener("tabNavigated", this._onTabNavigated);
    client.addListener("tabDetached", this._onTabDetached);

    client.connect((aType, aTraits) => {
      client.listTabs((aResponse) => {
        this._startChromeDebugging(client, aResponse.chromeDebugger, deferred.resolve);
      });
    });

    return deferred.promise;
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
    if (aType == "will-navigate") {
      DebuggerView._handleTabNavigation();

      
      DebuggerController.SourceScripts.clearCache();
      DebuggerController.Parser.clearCache();
      SourceUtils.clearCache();
      return;
    }

    this.ThreadState._handleTabNavigation();
    this.StackFrames._handleTabNavigation();
    this.SourceScripts._handleTabNavigation();
  },

  


  _onTabDetached: function() {
    this.shutdownDebugger();
  },

  









  _startDebuggingTab: function(aClient, aThreadActor, aCallback) {
    if (!aClient) {
      Cu.reportError("No client found!");
      return;
    }
    this.client = aClient;

    aClient.attachThread(aThreadActor, (aResponse, aThreadClient) => {
      if (!aThreadClient) {
        Cu.reportError("Couldn't attach to thread: " + aResponse.error);
        return;
      }
      this.activeThread = aThreadClient;

      this.ThreadState.connect();
      this.StackFrames.connect();
      this.SourceScripts.connect();
      aThreadClient.resume(this._ensureResumptionOrder);

      if (aCallback) {
        aCallback();
      }
    }, { useSourceMaps: Prefs.sourceMapsEnabled });
  },

  


  _ensureResumptionOrder: function(aResponse) {
    if (aResponse.error == "wrongOrder") {
      DebuggerView.Toolbar.showResumeWarning(aResponse.lastPausedUrl);
    }
  },

  









  _startChromeDebugging: function(aClient, aChromeDebugger, aCallback) {
    if (!aClient) {
      Cu.reportError("No client found!");
      return;
    }
    this.client = aClient;

    aClient.attachThread(aChromeDebugger, (aResponse, aThreadClient) => {
      if (!aThreadClient) {
        Cu.reportError("Couldn't attach to thread: " + aResponse.error);
        return;
      }
      this.activeThread = aThreadClient;

      this.ThreadState.connect();
      this.StackFrames.connect();
      this.SourceScripts.connect();
      aThreadClient.resume(this._ensureResumptionOrder);

      if (aCallback) {
        aCallback();
      }
    }, { useSourceMaps: Prefs.sourceMapsEnabled });
  },

  



  reconfigureThread: function(aUseSourceMaps) {
    this.client.reconfigureThread(aUseSourceMaps, (aResponse) => {
      if (aResponse.error) {
        let msg = "Couldn't reconfigure thread: " + aResponse.message;
        Cu.reportError(msg);
        dumpn(msg);
        return;
      }

      
      DebuggerView.Sources.empty();
      SourceUtils.clearCache();
      this.SourceScripts._handleTabNavigation();
      
      this.activeThread._clearFrames();
      this.activeThread.fillFrames(CALL_STACK_PAGE_SIZE);
    });
  },

  


  _quitApp: function() {
    let canceled = Cc["@mozilla.org/supports-PRBool;1"]
      .createInstance(Ci.nsISupportsPRBool);

    Services.obs.notifyObservers(canceled, "quit-application-requested", null);

    
    if (canceled.data) {
      return;
    }
    Services.startup.quit(Ci.nsIAppStartup.eAttemptQuit);
  },

  _isInitialized: false,
  _isDestroyed: false,
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
    this.activeThread.pauseOnExceptions(Prefs.pauseOnExceptions);
    this._handleTabNavigation();
  },

  


  disconnect: function() {
    if (!this.activeThread) {
      return;
    }
    dumpn("ThreadState is disconnecting...");
    this.activeThread.removeListener("paused", this._update);
    this.activeThread.removeListener("resumed", this._update);
  },

  


  _handleTabNavigation: function() {
    if (!this.activeThread) {
      return;
    }
    dumpn("Handling tab navigation in the ThreadState");
    this._update();
  },

  


  _update: function(aEvent) {
    DebuggerView.Toolbar.toggleResumeButtonState(this.activeThread.state);

    if (DebuggerController._target && (aEvent == "paused" || aEvent == "resumed")) {
      DebuggerController._target.emit("thread-" + aEvent);
    }
  }
};





function StackFrames() {
  this._onPaused = this._onPaused.bind(this);
  this._onResumed = this._onResumed.bind(this);
  this._onFrames = this._onFrames.bind(this);
  this._onFramesCleared = this._onFramesCleared.bind(this);
  this._afterFramesCleared = this._afterFramesCleared.bind(this);
  this._fetchScopeVariables = this._fetchScopeVariables.bind(this);
  this._fetchVarProperties = this._fetchVarProperties.bind(this);
  this._addVarExpander = this._addVarExpander.bind(this);
  this.evaluate = this.evaluate.bind(this);
}

StackFrames.prototype = {
  get activeThread() DebuggerController.activeThread,
  autoScopeExpand: false,
  currentFrame: null,
  syncedWatchExpressions: null,
  currentWatchExpressions: null,
  currentBreakpointLocation: null,
  currentEvaluation: null,
  currentException: null,
  currentReturnedValue: null,

  


  connect: function() {
    dumpn("StackFrames is connecting...");
    this.activeThread.addListener("paused", this._onPaused);
    this.activeThread.addListener("resumed", this._onResumed);
    this.activeThread.addListener("framesadded", this._onFrames);
    this.activeThread.addListener("framescleared", this._onFramesCleared);
    this._handleTabNavigation();
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
  },

  


  _handleTabNavigation: function() {
    dumpn("Handling tab navigation in the StackFrames");
    
  },

  







  _onPaused: function(aEvent, aPacket) {
    switch (aPacket.why.type) {
      
      case "breakpoint":
        this.currentBreakpointLocation = aPacket.frame.where;
        break;
      
      case "clientEvaluated":
        this.currentEvaluation = aPacket.why.frameFinished;
        break;
      
      case "exception":
        this.currentException = aPacket.why.exception;
        break;
      
      
      case "resumeLimit":
        if (!aPacket.why.frameFinished) {
          break;
        } else if (aPacket.why.frameFinished.throw) {
          this.currentException = aPacket.why.frameFinished.throw;
        } else if (aPacket.why.frameFinished.return) {
          this.currentReturnedValue = aPacket.why.frameFinished.return;
        }
        break;
    }

    this.activeThread.fillFrames(CALL_STACK_PAGE_SIZE);
    DebuggerView.editor.focus();
  },

  


  _onResumed: function() {
    DebuggerView.editor.setDebugLocation(-1);

    
    if (!this._isWatchExpressionsEvaluation) {
      this.currentWatchExpressions = this.syncedWatchExpressions;
    }
  },

  


  _onFrames: function() {
    
    if (!this.activeThread.cachedFrames.length) {
      return;
    }

    
    
    
    if (this.currentBreakpointLocation) {
      let { url, line } = this.currentBreakpointLocation;
      let breakpointClient = DebuggerController.Breakpoints.getBreakpoint(url, line);
      if (breakpointClient) {
        
        let conditionalExpression = breakpointClient.conditionalExpression;
        if (conditionalExpression) {
          
          
          
          this.evaluate(conditionalExpression, 0);
          this._isConditionalBreakpointEvaluation = true;
          return;
        }
      }
    }
    
    if (this._isConditionalBreakpointEvaluation) {
      this._isConditionalBreakpointEvaluation = false;
      
      
      if (VariablesView.isFalsy({ value: this.currentEvaluation.return })) {
        this.activeThread.resume(DebuggerController._ensureResumptionOrder);
        return;
      }
    }


    
    
    if (this.currentWatchExpressions) {
      
      
      this.evaluate(this.currentWatchExpressions, 0);
      this._isWatchExpressionsEvaluation = true;
      return;
    }
    
    if (this._isWatchExpressionsEvaluation) {
      this._isWatchExpressionsEvaluation = false;
      
      
      if (this.currentEvaluation.throw) {
        DebuggerView.WatchExpressions.removeExpressionAt(0);
        DebuggerController.StackFrames.syncWatchExpressions();
        return;
      }
      
      
      let topmostFrame = this.activeThread.cachedFrames[0];
      topmostFrame.watchExpressionsEvaluation = this.currentEvaluation.return;
    }


    
    DebuggerView.showInstrumentsPane();

    
    DebuggerView.StackFrames.empty();

    for (let frame of this.activeThread.cachedFrames) {
      this._addFrame(frame);
    }
    if (this.currentFrame == null) {
      DebuggerView.StackFrames.selectedDepth = 0;
    }
    if (this.activeThread.moreFrames) {
      DebuggerView.StackFrames.dirty = true;
    }
  },

  


  _onFramesCleared: function() {
    this.currentFrame = null;
    this.currentWatchExpressions = null;
    this.currentBreakpointLocation = null;
    this.currentEvaluation = null;
    this.currentException = null;
    this.currentReturnedValue = null;
    
    
    
    
    window.setTimeout(this._afterFramesCleared, FRAME_STEP_CLEAR_DELAY);
  },

  


  _afterFramesCleared: function() {
    
    if (this.activeThread.cachedFrames.length) {
      return;
    }
    DebuggerView.StackFrames.empty();
    DebuggerView.Sources.unhighlightBreakpoint();
    DebuggerView.WatchExpressions.toggleContents(true);
    DebuggerView.Variables.empty(0);
    window.dispatchEvent(document, "Debugger:AfterFramesCleared");
  },

  






  selectFrame: function(aDepth) {
    let frame = this.activeThread.cachedFrames[this.currentFrame = aDepth];
    if (!frame) {
      return;
    }
    let { environment, watchExpressionsEvaluation } = frame;
    let { url, line } = frame.where;

    
    if (!environment) {
      return;
    }

    
    DebuggerView.updateEditor(url, line);
    
    DebuggerView.Sources.highlightBreakpoint(url, line);
    
    DebuggerView.WatchExpressions.toggleContents(false);
    
    DebuggerView.Variables.createHierarchy();
    
    DebuggerView.Variables.empty();

    
    
    if (this.syncedWatchExpressions && watchExpressionsEvaluation) {
      let label = L10N.getStr("watchExpressionsScopeLabel");
      let scope = DebuggerView.Variables.addScope(label);

      
      scope.descriptorTooltip = false;
      scope.contextMenuId = "debuggerWatchExpressionsContextMenu";
      scope.separatorStr = L10N.getStr("watchExpressionsSeparatorLabel");
      scope.switch = DebuggerView.WatchExpressions.switchExpression;
      scope.delete = DebuggerView.WatchExpressions.deleteExpression;

      
      
      this._fetchWatchExpressions(scope, watchExpressionsEvaluation);
      scope.expand();
    }

    do {
      
      let label = StackFrameUtils.getScopeLabel(environment);
      let scope = DebuggerView.Variables.addScope(label);

      
      if (environment == frame.environment) {
        this._insertScopeFrameReferences(scope, frame);
        this._addScopeExpander(scope, environment);
        
        scope.expand();
      }
      
      else {
        this._addScopeExpander(scope, environment);
        this.autoScopeExpand && scope.expand();
      }
    } while ((environment = environment.parent));

    
    window.dispatchEvent(document, "Debugger:FetchedVariables");
    DebuggerView.Variables.commitHierarchy();
  },

  








  _addScopeExpander: function(aScope, aEnv) {
    aScope._sourceEnvironment = aEnv;

    
    aScope.addEventListener("mouseover", this._fetchScopeVariables, false);
    
    aScope.onexpand = this._fetchScopeVariables;
  },

  








  _addVarExpander: function(aVar, aGrip) {
    
    if (VariablesView.isPrimitive({ value: aGrip })) {
      return;
    }
    aVar._sourceGrip = aGrip;

    
    
    if (aVar.name == "window" || aVar.name == "this") {
      aVar.addEventListener("mouseover", this._fetchVarProperties, false);
    }
    
    aVar.onexpand = this._fetchVarProperties;
  },

  







  _fetchWatchExpressions: function(aScope, aExp) {
    
    if (aScope._fetched) {
      return;
    }
    aScope._fetched = true;

    
    this.activeThread.pauseGrip(aExp).getPrototypeAndProperties((aResponse) => {
      let ownProperties = aResponse.ownProperties;
      let totalExpressions = DebuggerView.WatchExpressions.itemCount;

      for (let i = 0; i < totalExpressions; i++) {
        let name = DebuggerView.WatchExpressions.getExpression(i);
        let expVal = ownProperties[i].value;
        let expRef = aScope.addVar(name, ownProperties[i]);
        this._addVarExpander(expRef, expVal);

        
        expRef.switch = null;
        expRef.delete = null;
        expRef.descriptorTooltip = true;
        expRef.separatorStr = L10N.getStr("variablesSeparatorLabel");
      }

      
      window.dispatchEvent(document, "Debugger:FetchedWatchExpressions");
      DebuggerView.Variables.commitHierarchy();
    });
  },

  






  _fetchScopeVariables: function(aScope) {
    
    if (aScope._fetched) {
      return;
    }
    aScope._fetched = true;
    let env = aScope._sourceEnvironment;

    switch (env.type) {
      case "with":
      case "object":
        
        this.activeThread.pauseGrip(env.object).getPrototypeAndProperties((aResponse) => {
          let { ownProperties, safeGetterValues } = aResponse;
          this._mergeSafeGetterValues(ownProperties, safeGetterValues);
          this._insertScopeVariables(ownProperties, aScope);

          
          window.dispatchEvent(document, "Debugger:FetchedVariables");
          DebuggerView.Variables.commitHierarchy();
        });
        break;
      case "block":
      case "function":
        
        this._insertScopeArguments(env.bindings.arguments, aScope);
        this._insertScopeVariables(env.bindings.variables, aScope);

        
        
        
        break;
      default:
        Cu.reportError("Unknown Debugger.Environment type: " + env.type);
        break;
    }
  },

  







  _insertScopeFrameReferences: function(aScope, aFrame) {
    
    if (this.currentException) {
      let excRef = aScope.addVar("<exception>", { value: this.currentException });
      this._addVarExpander(excRef, this.currentException);
    }
    
    if (this.currentReturnedValue) {
      let retRef = aScope.addVar("<return>", { value: this.currentReturnedValue });
      this._addVarExpander(retRef, this.currentReturnedValue);
    }
    
    if (aFrame.this) {
      let thisRef = aScope.addVar("this", { value: aFrame.this });
      this._addVarExpander(thisRef, aFrame.this);
    }
  },

  







  _insertScopeArguments: function(aArguments, aScope) {
    if (!aArguments) {
      return;
    }
    for (let argument of aArguments) {
      let name = Object.getOwnPropertyNames(argument)[0];
      let argRef = aScope.addVar(name, argument[name]);
      let argVal = argument[name].value;
      this._addVarExpander(argRef, argVal);
    }
  },

  







  _insertScopeVariables: function(aVariables, aScope) {
    if (!aVariables) {
      return;
    }
    let variableNames = Object.keys(aVariables);

    
    if (Prefs.variablesSortingEnabled) {
      variableNames.sort();
    }
    
    for (let name of variableNames) {
      let varRef = aScope.addVar(name, aVariables[name]);
      let varVal = aVariables[name].value;
      this._addVarExpander(varRef, varVal);
    }
  },

  






  _fetchVarProperties: function(aVar) {
    
    if (aVar._fetched) {
      return;
    }
    aVar._fetched = true;
    let grip = aVar._sourceGrip;

    this.activeThread.pauseGrip(grip).getPrototypeAndProperties((aResponse) => {
      let { ownProperties, prototype, safeGetterValues } = aResponse;
      let sortable = VariablesView.NON_SORTABLE_CLASSES.indexOf(grip.class) == -1;

      this._mergeSafeGetterValues(ownProperties, safeGetterValues);

      
      if (ownProperties) {
        aVar.addProperties(ownProperties, {
          
          sorted: sortable,
          
          callback: this._addVarExpander
        });
      }

      
      if (prototype && prototype.type != "null") {
        aVar.addProperty("__proto__", { value: prototype });
        
        this._addVarExpander(aVar.get("__proto__"), prototype);
      }

      
      aVar._retrieved = true;

      
      window.dispatchEvent(document, "Debugger:FetchedProperties");
      DebuggerView.Variables.commitHierarchy();
    });
  },

  










  _mergeSafeGetterValues: function(aOwnProperties, aSafeGetterValues) {
    
    
    for (let name of Object.keys(aSafeGetterValues)) {
      if (name in aOwnProperties) {
        aOwnProperties[name].getterValue = aSafeGetterValues[name].getterValue;
        aOwnProperties[name].getterPrototypeLevel =
          aSafeGetterValues[name].getterPrototypeLevel;
      } else {
        aOwnProperties[name] = aSafeGetterValues[name];
      }
    }
  },

  





  _addFrame: function(aFrame) {
    let depth = aFrame.depth;
    let { url, line } = aFrame.where;
    let frameLocation = NetworkHelper.convertToUnicode(unescape(url));
    let frameTitle = StackFrameUtils.getFrameTitle(aFrame);

    DebuggerView.StackFrames.addFrame(frameTitle, frameLocation, line, depth);
  },

  


  addMoreFrames: function() {
    this.activeThread.fillFrames(
      this.activeThread.cachedFrames.length + CALL_STACK_PAGE_SIZE);
  },

  


  syncWatchExpressions: function() {
    let list = DebuggerView.WatchExpressions.getExpressions();

    
    
    
    
    let sanitizedExpressions = list.map(function(str) {
      
      try {
        Parser.reflectionAPI.parse(str);
        return str; 
      } catch (e) {
        return "\"" + e.name + ": " + e.message + "\""; 
      }
    });

    if (sanitizedExpressions.length) {
      this.syncedWatchExpressions =
        this.currentWatchExpressions =
          "[" +
            sanitizedExpressions.map(function(str)
              "eval(\"" +
                "try {" +
                  
                  
                  
                  str.replace(/"/g, "\\$&") + "\" + " + "'\\n'" + " + \"" +
                "} catch (e) {" +
                  "e.name + ': ' + e.message;" + 
                "}" +
              "\")"
            ).join(",") +
          "]";
    } else {
      this.syncedWatchExpressions =
        this.currentWatchExpressions = null;
    }
    this.currentFrame = null;
    this._onFrames();
  },

  








  evaluate: function(aExpression, aFrame = this.currentFrame || 0) {
    let frame = this.activeThread.cachedFrames[aFrame];
    this.activeThread.eval(frame.actor, aExpression);
  }
};





function SourceScripts() {
  this._cache = new Map(); 
  this._onNewSource = this._onNewSource.bind(this);
  this._onNewGlobal = this._onNewGlobal.bind(this);
  this._onSourcesAdded = this._onSourcesAdded.bind(this);
  this._onFetch = this._onFetch.bind(this);
  this._onTimeout = this._onTimeout.bind(this);
  this._onFinished = this._onFinished.bind(this);
}

SourceScripts.prototype = {
  get activeThread() DebuggerController.activeThread,
  get debuggerClient() DebuggerController.client,
  _newSourceTimeout: null,

  


  connect: function() {
    dumpn("SourceScripts is connecting...");
    this.debuggerClient.addListener("newGlobal", this._onNewGlobal);
    this.debuggerClient.addListener("newSource", this._onNewSource);
    this._handleTabNavigation();
  },

  


  disconnect: function() {
    if (!this.activeThread) {
      return;
    }
    dumpn("SourceScripts is disconnecting...");
    window.clearTimeout(this._newSourceTimeout);
    this.debuggerClient.removeListener("newGlobal", this._onNewGlobal);
    this.debuggerClient.removeListener("newSource", this._onNewSource);
  },

  


  _handleTabNavigation: function() {
    if (!this.activeThread) {
      return;
    }
    dumpn("Handling tab navigation in the SourceScripts");
    window.clearTimeout(this._newSourceTimeout);

    
    
    this.activeThread.getSources(this._onSourcesAdded);
  },

  


  _onNewGlobal: function(aNotification, aPacket) {
    
    
  },

  


  _onNewSource: function(aNotification, aPacket) {
    
    if (NEW_SOURCE_IGNORED_URLS.indexOf(aPacket.source.url) != -1) {
      return;
    }

    
    DebuggerView.Sources.addSource(aPacket.source, { staged: false });

    let container = DebuggerView.Sources;
    let preferredValue = container.preferredValue;

    
    if (aPacket.source.url == preferredValue) {
      container.selectedValue = preferredValue;
    }
    
    else {
      window.clearTimeout(this._newSourceTimeout);
      this._newSourceTimeout = window.setTimeout(function() {
        
        
        if (!container.selectedValue) {
          container.selectedIndex = 0;
        }
      }, NEW_SOURCE_DISPLAY_DELAY);
    }

    
    
    DebuggerController.Breakpoints.updateEditorBreakpoints();
    DebuggerController.Breakpoints.updatePaneBreakpoints();

    
    window.dispatchEvent(document, "Debugger:AfterNewSource");
  },

  


  _onSourcesAdded: function(aResponse) {
    if (aResponse.error) {
      Cu.reportError(new Error("Error getting sources: " + aResponse.message));
      return;
    }

    
    for (let source of aResponse.sources) {
      
      if (NEW_SOURCE_IGNORED_URLS.indexOf(source.url) != -1) {
        continue;
      }
      DebuggerView.Sources.addSource(source, { staged: true });
    }

    let container = DebuggerView.Sources;
    let preferredValue = container.preferredValue;

    
    container.commit({ sorted: true });

    
    if (container.containsValue(preferredValue)) {
      container.selectedValue = preferredValue;
    }
    
    else if (!container.selectedValue) {
      container.selectedIndex = 0;
    }

    
    
    DebuggerController.Breakpoints.updateEditorBreakpoints();
    DebuggerController.Breakpoints.updatePaneBreakpoints();

    
    window.dispatchEvent(document, "Debugger:AfterSourcesAdded");
  },

  









  getText: function(aSource, aCallback, aTimeout) {
    
    if (aSource.loaded) {
      aCallback(aSource);
      return;
    }

    
    
    if (aTimeout) {
      var fetchTimeout = window.setTimeout(() => {
        aSource._fetchingTimedOut = true;
        aTimeout(aSource);
      }, FETCH_SOURCE_RESPONSE_DELAY);
    }

    
    this.activeThread.source(aSource).source((aResponse) => {
      if (aTimeout) {
        window.clearTimeout(fetchTimeout);
      }
      if (aResponse.error) {
        Cu.reportError("Error loading: " + aSource.url + "\n" + aResponse.message);
        return void aCallback(aSource);
      }
      aSource.loaded = true;
      aSource.text = aResponse.source;
      aCallback(aSource);
    });
  },

  





  getCache: function() {
    let sources = [];
    for (let source of this._cache) {
      sources.push(source);
    }
    return sources.sort(([first], [second]) => first > second);
  },

  


  clearCache: function() {
    this._cache.clear();
  },

  










  fetchSources: function(aUrls, aCallbacks = {}) {
    this._fetchQueue = new Set();
    this._fetchCallbacks = aCallbacks;

    
    for (let url of aUrls) {
      if (!this._cache.has(url)) {
        this._fetchQueue.add(url);
      }
    }

    
    if (this._fetchQueue.size == 0) {
      this._onFinished();
      return;
    }

    
    for (let url of this._fetchQueue) {
      let sourceItem = DebuggerView.Sources.getItemByValue(url);
      let sourceObject = sourceItem.attachment.source;
      this.getText(sourceObject, this._onFetch, this._onTimeout);
    }
  },

  





  _onFetch: function(aSource) {
    
    this._cache.set(aSource.url, aSource.text);

    
    this._fetchQueue.delete(aSource.url);

    
    
    if (aSource._fetchingTimedOut) {
      return;
    }

    
    if (this._fetchCallbacks.onFetch) {
      this._fetchCallbacks.onFetch(aSource);
    }

    
    if (this._fetchQueue.size == 0) {
      this._onFinished();
    }
  },

  





  _onTimeout: function(aSource) {
    
    this._fetchQueue.delete(aSource.url);

    
    if (this._fetchCallbacks.onTimeout) {
      this._fetchCallbacks.onTimeout(aSource);
    }

    
    if (this._fetchQueue.size == 0) {
      this._onFinished();
    }
  },

  


  _onFinished: function() {
    
    if (this._fetchCallbacks.onFinished) {
      this._fetchCallbacks.onFinished();
    }
  },

  _cache: null,
  _fetchQueue: null,
  _fetchCallbacks: null
};




function Breakpoints() {
  this._onEditorBreakpointChange = this._onEditorBreakpointChange.bind(this);
  this._onEditorBreakpointAdd = this._onEditorBreakpointAdd.bind(this);
  this._onEditorBreakpointRemove = this._onEditorBreakpointRemove.bind(this);
  this.addBreakpoint = this.addBreakpoint.bind(this);
  this.removeBreakpoint = this.removeBreakpoint.bind(this);
  this.getBreakpoint = this.getBreakpoint.bind(this);
}

Breakpoints.prototype = {
  get activeThread() DebuggerController.ThreadState.activeThread,
  get editor() DebuggerView.editor,

  





  store: {},

  











  _skipEditorBreakpointCallbacks: false,

  


  initialize: function() {
    this.editor.addEventListener(
      SourceEditor.EVENTS.BREAKPOINT_CHANGE, this._onEditorBreakpointChange);
  },

  


  destroy: function() {
    this.editor.removeEventListener(
      SourceEditor.EVENTS.BREAKPOINT_CHANGE, this._onEditorBreakpointChange);

    for each (let breakpointClient in this.store) {
      this.removeBreakpoint(breakpointClient);
    }
  },

  






  _onEditorBreakpointChange: function(aEvent) {
    if (this._skipEditorBreakpointCallbacks) {
      return;
    }
    this._skipEditorBreakpointCallbacks = true;
    aEvent.added.forEach(this._onEditorBreakpointAdd, this);
    aEvent.removed.forEach(this._onEditorBreakpointRemove, this);
    this._skipEditorBreakpointCallbacks = false;
  },

  





  _onEditorBreakpointAdd: function(aEditorBreakpoint) {
    let url = DebuggerView.Sources.selectedValue;
    let line = aEditorBreakpoint.line + 1;

    this.addBreakpoint({ url: url, line: line }, (aBreakpointClient) => {
      
      
      
      if (aBreakpointClient.actualLocation) {
        this.editor.removeBreakpoint(line - 1);
        this.editor.addBreakpoint(aBreakpointClient.actualLocation.line - 1);
      }
    });
  },

  





  _onEditorBreakpointRemove: function(aEditorBreakpoint) {
    let url = DebuggerView.Sources.selectedValue;
    let line = aEditorBreakpoint.line + 1;

    this.removeBreakpoint(this.getBreakpoint(url, line));
  },

  




  updateEditorBreakpoints: function() {
    for each (let breakpointClient in this.store) {
      if (DebuggerView.Sources.selectedValue == breakpointClient.location.url) {
        this._showBreakpoint(breakpointClient, {
          noPaneUpdate: true,
          noPaneHighlight: true
        });
      }
    }
  },

  




  updatePaneBreakpoints: function() {
    for each (let breakpointClient in this.store) {
      if (DebuggerView.Sources.containsValue(breakpointClient.location.url)) {
        this._showBreakpoint(breakpointClient, {
          noEditorUpdate: true,
          noPaneHighlight: true
        });
      }
    }
  },

  




















  addBreakpoint: function(aLocation, aCallback, aFlags = {}) {
    let breakpointClient = this.getBreakpoint(aLocation.url, aLocation.line);

    
    if (breakpointClient) {
      aCallback && aCallback(breakpointClient);
      return;
    }

    this.activeThread.setBreakpoint(aLocation, (aResponse, aBreakpointClient) => {
      let { url, line } = aResponse.actualLocation || aLocation;

      
      
      if (this.getBreakpoint(url, line)) {
        this._hideBreakpoint(aBreakpointClient);
        return;
      }

      
      
      if (aResponse.actualLocation) {
        
        aBreakpointClient.requestedLocation = {
          url: aBreakpointClient.location.url,
          line: aBreakpointClient.location.line
        };
        
        aBreakpointClient.actualLocation = aResponse.actualLocation;
        
        aBreakpointClient.location.url = aResponse.actualLocation.url;
        aBreakpointClient.location.line = aResponse.actualLocation.line;
      }

      
      this.store[aBreakpointClient.actor] = aBreakpointClient;

      
      aBreakpointClient.conditionalExpression = aFlags.conditionalExpression;

      
      
      aBreakpointClient.lineText = DebuggerView.getEditorLine(line - 1).trim();

      
      this._showBreakpoint(aBreakpointClient, aFlags);

      
      aCallback && aCallback(aBreakpointClient, aResponse.error);
    });
  },

  











  removeBreakpoint: function(aBreakpointClient, aCallback, aFlags = {}) {
    let breakpointActor = (aBreakpointClient || {}).actor;

    
    if (!this.store[breakpointActor]) {
      aCallback && aCallback(aBreakpointClient.location);
      return;
    }

    aBreakpointClient.remove(() => {
      
      delete this.store[breakpointActor];

      
      this._hideBreakpoint(aBreakpointClient, aFlags);

      
      aCallback && aCallback(aBreakpointClient.location);
    });
  },

  







  _showBreakpoint: function(aBreakpointClient, aFlags = {}) {
    let currentSourceUrl = DebuggerView.Sources.selectedValue;
    let { url, line } = aBreakpointClient.location;

    
    if (!aFlags.noEditorUpdate) {
      if (url == currentSourceUrl) {
        this._skipEditorBreakpointCallbacks = true;
        this.editor.addBreakpoint(line - 1);
        this._skipEditorBreakpointCallbacks = false;
      }
    }
    
    if (!aFlags.noPaneUpdate) {
      DebuggerView.Sources.addBreakpoint({
        sourceLocation: url,
        lineNumber: line,
        lineText: aBreakpointClient.lineText,
        actor: aBreakpointClient.actor,
        openPopupFlag: aFlags.openPopup
      });
    }
    
    if (!aFlags.noPaneHighlight) {
      DebuggerView.Sources.highlightBreakpoint(url, line, aFlags);
    }
  },

  







  _hideBreakpoint: function(aBreakpointClient, aFlags = {}) {
    let currentSourceUrl = DebuggerView.Sources.selectedValue;
    let { url, line } = aBreakpointClient.location;

    
    if (!aFlags.noEditorUpdate) {
      if (url == currentSourceUrl) {
        this._skipEditorBreakpointCallbacks = true;
        this.editor.removeBreakpoint(line - 1);
        this._skipEditorBreakpointCallbacks = false;
      }
    }
    
    if (!aFlags.noPaneUpdate) {
      DebuggerView.Sources.removeBreakpoint(url, line);
    }
  },

  









  getBreakpoint: function(aUrl, aLine) {
    for each (let breakpointClient in this.store) {
      if (breakpointClient.location.url == aUrl &&
          breakpointClient.location.line == aLine) {
        return breakpointClient;
      }
    }
    return null;
  }
};




let L10N = new ViewHelpers.L10N(DBG_STRINGS_URI);




let Prefs = new ViewHelpers.Prefs("devtools.debugger", {
  chromeDebuggingHost: ["Char", "chrome-debugging-host"],
  chromeDebuggingPort: ["Int", "chrome-debugging-port"],
  windowX: ["Int", "ui.win-x"],
  windowY: ["Int", "ui.win-y"],
  windowWidth: ["Int", "ui.win-width"],
  windowHeight: ["Int", "ui.win-height"],
  sourcesWidth: ["Int", "ui.panes-sources-width"],
  instrumentsWidth: ["Int", "ui.panes-instruments-width"],
  pauseOnExceptions: ["Bool", "ui.pause-on-exceptions"],
  panesVisibleOnStartup: ["Bool", "ui.panes-visible-on-startup"],
  variablesSortingEnabled: ["Bool", "ui.variables-sorting-enabled"],
  variablesOnlyEnumVisible: ["Bool", "ui.variables-only-enum-visible"],
  variablesSearchboxVisible: ["Bool", "ui.variables-searchbox-visible"],
  sourceMapsEnabled: ["Bool", "source-maps-enabled"],
  remoteHost: ["Char", "remote-host"],
  remotePort: ["Int", "remote-port"],
  remoteAutoConnect: ["Bool", "remote-autoconnect"],
  remoteConnectionRetries: ["Int", "remote-connection-retries"],
  remoteTimeout: ["Int", "remote-timeout"]
});





XPCOMUtils.defineLazyGetter(window, "_isRemoteDebugger", function() {
  
  return !(window.frameElement instanceof XULElement) &&
         !!window._remoteFlag;
});





XPCOMUtils.defineLazyGetter(window, "_isChromeDebugger", function() {
  
  return !(window.frameElement instanceof XULElement) &&
         !window._remoteFlag;
});




DebuggerController.initialize();
DebuggerController.Parser = new Parser();
DebuggerController.ThreadState = new ThreadState();
DebuggerController.StackFrames = new StackFrames();
DebuggerController.SourceScripts = new SourceScripts();
DebuggerController.Breakpoints = new Breakpoints();




Object.defineProperties(window, {
  "create": {
    get: function() ViewHelpers.create,
  },
  "dispatchEvent": {
    get: function() ViewHelpers.dispatchEvent,
  },
  "editor": {
    get: function() DebuggerView.editor
  },
  "gClient": {
    get: function() DebuggerController.client
  },
  "gThreadClient": {
    get: function() DebuggerController.activeThread
  },
  "gThreadState": {
    get: function() DebuggerController.ThreadState
  },
  "gStackFrames": {
    get: function() DebuggerController.StackFrames
  },
  "gSourceScripts": {
    get: function() DebuggerController.SourceScripts
  },
  "gBreakpoints": {
    get: function() DebuggerController.Breakpoints
  },
  "gCallStackPageSize": {
    get: function() CALL_STACK_PAGE_SIZE,
  }
});





function dumpn(str) {
  if (wantLogging) {
    dump("DBG-FRONTEND: " + str + "\n");
  }
}

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");
