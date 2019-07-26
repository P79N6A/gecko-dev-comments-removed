




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const NEW_SOURCE_IGNORED_URLS = ["debugger eval code", "self-hosted"];
const NEW_SOURCE_DISPLAY_DELAY = 200; 
const FETCH_SOURCE_RESPONSE_DELAY = 50; 
const FRAME_STEP_CLEAR_DELAY = 100; 
const CALL_STACK_PAGE_SIZE = 25; 
const VARIABLES_VIEW_NON_SORTABLE = [
  "Array",
  "Int8Array",
  "Uint8Array",
  "Int16Array",
  "Uint16Array",
  "Int32Array",
  "Uint32Array",
  "Float32Array",
  "Float64Array"
];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
Cu.import("resource:///modules/source-editor.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");
Cu.import("resource:///modules/devtools/VariablesView.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
  "Reflect", "resource://gre/modules/reflect.jsm");




let DebuggerController = {
  


  initialize: function DC_initialize() {
    dumpn("Initializing the DebuggerController");
    this._startupDebugger = this._startupDebugger.bind(this);
    this._shutdownDebugger = this._shutdownDebugger.bind(this);
    this._onTabNavigated = this._onTabNavigated.bind(this);
    this._onTabDetached = this._onTabDetached.bind(this);

    window.addEventListener("load", this._startupDebugger, true);
    window.addEventListener("unload", this._shutdownDebugger, true);
  },

  


  _startupDebugger: function DC__startupDebugger() {
    if (this._isInitialized) {
      return;
    }
    this._isInitialized = true;
    window.removeEventListener("load", this._startupDebugger, true);

    DebuggerView.initialize(function() {
      DebuggerView._isInitialized = true;

      window.dispatchEvent("Debugger:Loaded");
      this._connect();
    }.bind(this));
  },

  


  _shutdownDebugger: function DC__shutdownDebugger() {
    if (this._isDestroyed || !DebuggerView._isInitialized) {
      return;
    }
    this._isDestroyed = true;
    window.removeEventListener("unload", this._shutdownDebugger, true);

    DebuggerView.destroy(function() {
      DebuggerView._isDestroyed = true;
      this.SourceScripts.disconnect();
      this.StackFrames.disconnect();
      this.ThreadState.disconnect();

      this._disconnect();
      window.dispatchEvent("Debugger:Unloaded");
      window._isChromeDebugger && this._quitApp();
    }.bind(this));
  },

  






  _prepareConnection: function DC__prepareConnection() {
    
    if (this._remoteConnectionTry === Prefs.remoteConnectionRetries) {
      Services.prompt.alert(null,
        L10N.getStr("remoteDebuggerPromptTitle"),
        L10N.getStr("remoteDebuggerConnectionFailedMessage"));

      
      
      this._shutdownDebugger();
      return false;
    }

    
    if (!Prefs.remoteAutoConnect) {
      let prompt = new RemoteDebuggerPrompt();
      let result = prompt.show(!!this._remoteConnectionTimeout);

      
      
      if (!result) {
        this._shutdownDebugger();
        return false;
      }

      Prefs.remoteHost = prompt.remote.host;
      Prefs.remotePort = prompt.remote.port;
      Prefs.remoteAutoConnect = prompt.remote.auto;
    }

    
    
    this._remoteConnectionTry = ++this._remoteConnectionTry || 1;
    this._remoteConnectionTimeout = window.setTimeout(function() {
      
      if (!this.activeThread) {
        this._onRemoteConnectionTimeout();
        this._connect();
      }
    }.bind(this), Prefs.remoteTimeout);

    
    return true;
  },

  


  _onRemoteConnectionTimeout: function DC__onRemoteConnectionTimeout() {
    Cu.reportError("Couldn't connect to " +
      Prefs.remoteHost + ":" + Prefs.remotePort);
  },

  



  _connect: function DC__connect() {
    function callback() {
      window.dispatchEvent("Debugger:Connected");
    }

    if (!window._isChromeDebugger) {
      let client = this.client = this._target.client;
      this._target.on("close", this._onTabDetached);
      this._target.on("navigate", this._onTabNavigated);
      this._target.on("will-navigate", this._onTabNavigated);

      if (this._target.chrome) {
        let dbg = this._target.form.chromeDebugger;
        this._startChromeDebugging(client, dbg, callback);
      } else {
        this._startDebuggingTab(client, this._target.form, callback);
      }
      return;
    }

    
    let transport = debuggerSocketConnect(Prefs.remoteHost, Prefs.remotePort);

    let client = this.client = new DebuggerClient(transport);
    client.addListener("tabNavigated", this._onTabNavigated);
    client.addListener("tabDetached", this._onTabDetached);

    client.connect(function(aType, aTraits) {
      client.listTabs(function(aResponse) {
        this._startChromeDebugging(client, aResponse.chromeDebugger, callback);
      }.bind(this));
    }.bind(this));
  },

  


  _disconnect: function DC__disconnect() {
    
    if (!this.client) {
      return;
    }
    this.client.removeListener("tabNavigated", this._onTabNavigated);
    this.client.removeListener("tabDetached", this._onTabDetached);

    
    
    if (window._isChromeDebugger) {
      this.client.close();
    }

    this.client = null;
    this.tabClient = null;
    this.activeThread = null;
  },

  







  _onTabNavigated: function DC__onTabNavigated(aType, aPacket) {
    if (aPacket.state == "start") {
      DebuggerView._handleTabNavigation();
      return;
    }

    this.ThreadState._handleTabNavigation();
    this.StackFrames._handleTabNavigation();
    this.SourceScripts._handleTabNavigation();
  },

  


  _onTabDetached: function DC__onTabDetached() {
    this._shutdownDebugger();
  },

  







  _startDebuggingTab: function DC__startDebuggingTab(aClient, aTabGrip, aCallback) {
    if (!aClient) {
      Cu.reportError("No client found!");
      return;
    }
    this.client = aClient;

    aClient.attachTab(aTabGrip.actor, function(aResponse, aTabClient) {
      if (!aTabClient) {
        Cu.reportError("No tab client found!");
        return;
      }
      this.tabClient = aTabClient;

      aClient.attachThread(aResponse.threadActor, function(aResponse, aThreadClient) {
        if (!aThreadClient) {
          Cu.reportError("Couldn't attach to thread: " + aResponse.error);
          return;
        }
        this.activeThread = aThreadClient;

        this.ThreadState.connect();
        this.StackFrames.connect();
        this.SourceScripts.connect();
        aThreadClient.resume();

        if (aCallback) {
          aCallback();
        }
      }.bind(this));
    }.bind(this));
  },

  







  _startChromeDebugging: function DC__startChromeDebugging(aClient, aChromeDebugger, aCallback) {
    if (!aClient) {
      Cu.reportError("No client found!");
      return;
    }
    this.client = aClient;

    aClient.attachThread(aChromeDebugger, function(aResponse, aThreadClient) {
      if (!aThreadClient) {
        Cu.reportError("Couldn't attach to thread: " + aResponse.error);
        return;
      }
      this.activeThread = aThreadClient;

      this.ThreadState.connect();
      this.StackFrames.connect();
      this.SourceScripts.connect();
      aThreadClient.resume();

      if (aCallback) {
        aCallback();
      }
    }.bind(this));
  },

  


  _quitApp: function DC__quitApp() {
    let canceled = Cc["@mozilla.org/supports-PRBool;1"]
      .createInstance(Ci.nsISupportsPRBool);

    Services.obs.notifyObservers(canceled, "quit-application-requested", null);

    
    if (canceled.data) {
      return;
    }
    Services.startup.quit(Ci.nsIAppStartup.eAttemptQuit);
  },

  







  dispatchEvent: function DC_dispatchEvent(aType, aDetail) {
    let evt = document.createEvent("CustomEvent");
    evt.initCustomEvent(aType, true, false, aDetail);
    document.documentElement.dispatchEvent(evt);
  }
};





function ThreadState() {
  this._update = this._update.bind(this);
}

ThreadState.prototype = {
  get activeThread() DebuggerController.activeThread,

  


  connect: function TS_connect() {
    dumpn("ThreadState is connecting...");
    this.activeThread.addListener("paused", this._update);
    this.activeThread.addListener("resumed", this._update);
    this.activeThread.addListener("detached", this._update);
    this.activeThread.pauseOnExceptions(Prefs.pauseOnExceptions);
    this._handleTabNavigation();
  },

  


  disconnect: function TS_disconnect() {
    if (!this.activeThread) {
      return;
    }
    dumpn("ThreadState is disconnecting...");
    this.activeThread.removeListener("paused", this._update);
    this.activeThread.removeListener("resumed", this._update);
    this.activeThread.removeListener("detached", this._update);
  },

  


  _handleTabNavigation: function TS__handleTabNavigation() {
    if (!this.activeThread) {
      return;
    }
    dumpn("Handling tab navigation in the ThreadState");
    this._update(this.activeThread.state);
  },

  


  _update: function TS__update(aEvent) {
    DebuggerView.Toolbar.toggleResumeButtonState(this.activeThread.state);

    if (DebuggerController._target &&
        (aEvent == "paused" || aEvent == "resumed")) {
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

  


  connect: function SF_connect() {
    dumpn("StackFrames is connecting...");
    this.activeThread.addListener("paused", this._onPaused);
    this.activeThread.addListener("resumed", this._onResumed);
    this.activeThread.addListener("framesadded", this._onFrames);
    this.activeThread.addListener("framescleared", this._onFramesCleared);
    this._handleTabNavigation();
  },

  


  disconnect: function SF_disconnect() {
    if (!this.activeThread) {
      return;
    }
    dumpn("StackFrames is disconnecting...");
    this.activeThread.removeListener("paused", this._onPaused);
    this.activeThread.removeListener("resumed", this._onResumed);
    this.activeThread.removeListener("framesadded", this._onFrames);
    this.activeThread.removeListener("framescleared", this._onFramesCleared);
  },

  


  _handleTabNavigation: function SF__handleTabNavigation() {
    dumpn("Handling tab navigation in the StackFrames");
    
  },

  







  _onPaused: function SF__onPaused(aEvent, aPacket) {
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
    }

    this.activeThread.fillFrames(CALL_STACK_PAGE_SIZE);
    DebuggerView.editor.focus();
  },

  


  _onResumed: function SF__onResumed() {
    DebuggerView.editor.setDebugLocation(-1);

    
    if (!this._isWatchExpressionsEvaluation) {
      this.currentWatchExpressions = this.syncedWatchExpressions;
    }
  },

  


  _onFrames: function SF__onFrames() {
    
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
        this.activeThread.resume();
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


    
    DebuggerView.StackFrames.empty();

    for (let frame of this.activeThread.cachedFrames) {
      this._addFrame(frame);
    }
    if (this.currentFrame == null) {
      this.selectFrame(0);
    }
    if (this.activeThread.moreFrames) {
      DebuggerView.StackFrames.dirty = true;
    }
  },

  


  _onFramesCleared: function SF__onFramesCleared() {
    this.currentFrame = null;
    this.currentWatchExpressions = null;
    this.currentBreakpointLocation = null;
    this.currentEvaluation = null;
    this.currentException = null;
    
    
    
    
    window.setTimeout(this._afterFramesCleared, FRAME_STEP_CLEAR_DELAY);
  },

  


  _afterFramesCleared: function SF__afterFramesCleared() {
    
    if (this.activeThread.cachedFrames.length) {
      return;
    }
    DebuggerView.StackFrames.empty();
    DebuggerView.Variables.empty(0);
    DebuggerView.Breakpoints.unhighlightBreakpoint();
    DebuggerView.WatchExpressions.toggleContents(true);
    window.dispatchEvent("Debugger:AfterFramesCleared");
  },

  






  selectFrame: function SF_selectFrame(aDepth) {
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
    
    DebuggerView.StackFrames.highlightFrame(aDepth);
    
    DebuggerView.Breakpoints.highlightBreakpoint(url, line);
    
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
      
      let label = this._getScopeLabel(environment);
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
    } while (environment = environment.parent);

    
    window.dispatchEvent("Debugger:FetchedVariables");
    DebuggerView.Variables.commitHierarchy();
  },

  








  _addScopeExpander: function SF__addScopeExpander(aScope, aEnv) {
    aScope._sourceEnvironment = aEnv;

    
    aScope.addEventListener("mouseover", this._fetchScopeVariables, false);
    
    aScope.onexpand = this._fetchScopeVariables;
  },

  








  _addVarExpander: function SF__addVarExpander(aVar, aGrip) {
    
    if (VariablesView.isPrimitive({ value: aGrip })) {
      return;
    }
    aVar._sourceGrip = aGrip;

    
    
    if (aVar.name == "window" || aVar.name == "this") {
      aVar.addEventListener("mouseover", this._fetchVarProperties, false);
    }
    
    aVar.onexpand = this._fetchVarProperties;
  },

  







  _fetchWatchExpressions: function SF__fetchWatchExpressions(aScope, aExp) {
    
    if (aScope._fetched) {
      return;
    }
    aScope._fetched = true;

    
    this.activeThread.pauseGrip(aExp).getPrototypeAndProperties(function(aResponse) {
      let ownProperties = aResponse.ownProperties;
      let totalExpressions = DebuggerView.WatchExpressions.totalItems;

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

      
      window.dispatchEvent("Debugger:FetchedWatchExpressions");
      DebuggerView.Variables.commitHierarchy();
    }.bind(this));
  },

  






  _fetchScopeVariables: function SF__fetchScopeVariables(aScope) {
    
    if (aScope._fetched) {
      return;
    }
    aScope._fetched = true;
    let env = aScope._sourceEnvironment;

    switch (env.type) {
      case "with":
      case "object":
        
        this.activeThread.pauseGrip(env.object).getPrototypeAndProperties(function(aResponse) {
          this._insertScopeVariables(aResponse.ownProperties, aScope);

          
          window.dispatchEvent("Debugger:FetchedVariables");
          DebuggerView.Variables.commitHierarchy();
        }.bind(this));
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

  







  _insertScopeFrameReferences: function SF__insertScopeFrameReferences(aScope, aFrame) {
    
    if (this.currentException) {
      let excRef = aScope.addVar("<exception>", { value: this.currentException });
      this._addVarExpander(excRef, this.currentException);
    }
    
    if (aFrame.this) {
      let thisRef = aScope.addVar("this", { value: aFrame.this });
      this._addVarExpander(thisRef, aFrame.this);
    }
  },

  







  _insertScopeArguments: function SF__insertScopeArguments(aArguments, aScope) {
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

  







  _insertScopeVariables: function SF__insertScopeVariables(aVariables, aScope) {
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

  






  _fetchVarProperties: function SF__fetchVarProperties(aVar) {
    
    if (aVar._fetched) {
      return;
    }
    aVar._fetched = true;
    let grip = aVar._sourceGrip;

    this.activeThread.pauseGrip(grip).getPrototypeAndProperties(function(aResponse) {
      let { ownProperties, prototype } = aResponse;
      let sortable = VARIABLES_VIEW_NON_SORTABLE.indexOf(grip.class) == -1;

      
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

      
      window.dispatchEvent("Debugger:FetchedProperties");
      DebuggerView.Variables.commitHierarchy();
    }.bind(this));
  },

  







  _getScopeLabel: function SV__getScopeLabel(aEnv) {
    let name = "";

    
    if (!aEnv.parent) {
      name = L10N.getStr("globalScopeLabel");
    }
    
    else {
      name = aEnv.type.charAt(0).toUpperCase() + aEnv.type.slice(1);
    }

    let label = L10N.getFormatStr("scopeLabel", [name]);
    switch (aEnv.type) {
      case "with":
      case "object":
        label += " [" + aEnv.object.class + "]";
        break;
      case "function":
        let f = aEnv.function;
        label += " [" + (f.name || f.userDisplayName || f.displayName ||
                         "(anonymous)") + "]";
        break;
    }
    return label;
  },

  





  _addFrame: function SF__addFrame(aFrame) {
    let depth = aFrame.depth;
    let { url, line } = aFrame.where;

    let startText = StackFrameUtils.getFrameTitle(aFrame);
    let endText = SourceUtils.getSourceLabel(url) + ":" + line;
    DebuggerView.StackFrames.addFrame(startText, endText, depth);
  },

  


  addMoreFrames: function SF_addMoreFrames() {
    this.activeThread.fillFrames(
      this.activeThread.cachedFrames.length + CALL_STACK_PAGE_SIZE);
  },

  


  syncWatchExpressions: function SF_syncWatchExpressions() {
    let list = DebuggerView.WatchExpressions.getExpressions();

    
    
    
    
    let sanitizedExpressions = list.map(function(str) {
      
      try {
        Reflect.parse(str);
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

  








  evaluate: function SF_evaluate(aExpression, aFrame = this.currentFrame || 0) {
    let frame = this.activeThread.cachedFrames[aFrame];
    this.activeThread.eval(frame.actor, aExpression);
  }
};





function SourceScripts() {
  this._onNewSource = this._onNewSource.bind(this);
  this._onNewGlobal = this._onNewGlobal.bind(this);
  this._onSourcesAdded = this._onSourcesAdded.bind(this);
}

SourceScripts.prototype = {
  get activeThread() DebuggerController.activeThread,
  get debuggerClient() DebuggerController.client,

  


  connect: function SS_connect() {
    dumpn("SourceScripts is connecting...");
    this.debuggerClient.addListener("newGlobal", this._onNewGlobal);
    this.activeThread.addListener("newSource", this._onNewSource);
    this._handleTabNavigation();
  },

  


  disconnect: function SS_disconnect() {
    if (!this.activeThread) {
      return;
    }
    dumpn("SourceScripts is disconnecting...");
    window.clearTimeout(this._newSourceTimeout);
    this.debuggerClient.removeListener("newGlobal", this._onNewGlobal);
    this.activeThread.removeListener("newSource", this._onNewSource);
  },

  


  _handleTabNavigation: function SS__handleTabNavigation() {
    if (!this.activeThread) {
      return;
    }
    dumpn("Handling tab navigation in the SourceScripts");
    window.clearTimeout(this._newSourceTimeout);

    
    
    this.activeThread.getSources(this._onSourcesAdded);
  },

  


  _onNewSource: function SS__onNewSource(aNotification, aPacket) {
    
    if (NEW_SOURCE_IGNORED_URLS.indexOf(aPacket.url) != -1) {
      return;
    }

    
    this._addSource({
      url: aPacket.source.url,
      source: aPacket.source
    }, {
      forced: true
    });

    let container = DebuggerView.Sources;
    let preferredValue = container.preferredValue;

    
    if (aPacket.url == preferredValue) {
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

    
    window.dispatchEvent("Debugger:AfterNewScript");
  },

  


  _onNewGlobal: function SS__onNewGlobal(aNotification, aPacket) {
    
    
  },

  


  _onSourcesAdded: function SS__onSourcesAdded(aResponse) {
    if (aResponse.error) {
      Cu.reportError(new Error("Error getting sources: " + aResponse.error));
      return;
    }

    
    for (let source of aResponse.sources) {
      
      if (NEW_SOURCE_IGNORED_URLS.indexOf(source.url) != -1) {
        continue;
      }
      this._addSource({
        url: source.url,
        source: source
      });
    }

    let container = DebuggerView.Sources;
    let preferredValue = container.preferredValue;

    
    container.commit();

    
    if (container.containsValue(preferredValue)) {
      container.selectedValue = preferredValue;
    }
    
    else if (!container.selectedValue) {
      container.selectedIndex = 0;
    }

    
    
    DebuggerController.Breakpoints.updateEditorBreakpoints();
    DebuggerController.Breakpoints.updatePaneBreakpoints();

    
    window.dispatchEvent("Debugger:AfterSourcesAdded");
  },

  








  _addSource: function SS__addSource(aSource, aOptions = {}) {
    let url = aSource.url;
    let label = SourceUtils.getSourceLabel(url);

    DebuggerView.Sources.push(label, url, {
      forced: aOptions.forced,
      tooltip: url,
      attachment: aSource
    });
  },

  









  getText: function SS_getText(aSource, aCallback, aOnTimeout) {
    
    if (aSource.loaded) {
      aCallback(aSource.source.url, aSource.text);
      return;
    }

    
    
    if (aOnTimeout) {
      var fetchTimeout = window.setTimeout(aOnTimeout, FETCH_SOURCE_RESPONSE_DELAY);
    }

    
    this.activeThread.source(aSource.source).source(function(aResponse) {
      window.clearTimeout(fetchTimeout);

      if (aResponse.error) {
        Cu.reportError("Error loading " + aSource.source.url + "\n" + aResponse.error);
        aCallback(aSource.source.url, "");
        return;
      }
      aSource.loaded = true;
      aSource.text = aResponse.source;
      aCallback(aSource.source.url, aResponse.source);
    });
  }
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

  


  initialize: function BP_initialize() {
    this.editor.addEventListener(
      SourceEditor.EVENTS.BREAKPOINT_CHANGE, this._onEditorBreakpointChange);
  },

  


  destroy: function BP_destroy() {
    this.editor.removeEventListener(
      SourceEditor.EVENTS.BREAKPOINT_CHANGE, this._onEditorBreakpointChange);

    for each (let breakpointClient in this.store) {
      this.removeBreakpoint(breakpointClient);
    }
  },

  






  _onEditorBreakpointChange: function BP__onEditorBreakpointChange(aEvent) {
    if (this._skipEditorBreakpointCallbacks) {
      return;
    }
    this._skipEditorBreakpointCallbacks = true;
    aEvent.added.forEach(this._onEditorBreakpointAdd, this);
    aEvent.removed.forEach(this._onEditorBreakpointRemove, this);
    this._skipEditorBreakpointCallbacks = false;
  },

  





  _onEditorBreakpointAdd: function BP__onEditorBreakpointAdd(aEditorBreakpoint) {
    let url = DebuggerView.Sources.selectedValue;
    let line = aEditorBreakpoint.line + 1;

    this.addBreakpoint({ url: url, line: line }, function(aBreakpointClient) {
      
      
      
      if (aBreakpointClient.actualLocation) {
        this.editor.removeBreakpoint(line - 1);
        this.editor.addBreakpoint(aBreakpointClient.actualLocation.line - 1);
      }
    }.bind(this));
  },

  





  _onEditorBreakpointRemove: function BP__onEditorBreakpointRemove(aEditorBreakpoint) {
    let url = DebuggerView.Sources.selectedValue;
    let line = aEditorBreakpoint.line + 1;

    this.removeBreakpoint(this.getBreakpoint(url, line));
  },

  




  updateEditorBreakpoints: function BP_updateEditorBreakpoints() {
    for each (let breakpointClient in this.store) {
      if (DebuggerView.Sources.selectedValue == breakpointClient.location.url) {
        this._showBreakpoint(breakpointClient, {
          noPaneUpdate: true,
          noPaneHighlight: true
        });
      }
    }
  },

  




  updatePaneBreakpoints: function BP_updatePaneBreakpoints() {
    for each (let breakpointClient in this.store) {
      if (DebuggerView.Sources.containsValue(breakpointClient.location.url)) {
        this._showBreakpoint(breakpointClient, {
          noEditorUpdate: true,
          noPaneHighlight: true
        });
      }
    }
  },

  




















  addBreakpoint:
  function BP_addBreakpoint(aLocation, aCallback, aFlags = {}) {
    let breakpointClient = this.getBreakpoint(aLocation.url, aLocation.line);

    
    if (breakpointClient) {
      aCallback && aCallback(breakpointClient);
      return;
    }

    this.activeThread.setBreakpoint(aLocation, function(aResponse, aBreakpointClient) {
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

      
      
      aBreakpointClient.lineText = DebuggerView.getEditorLine(line - 1);
      aBreakpointClient.lineInfo = SourceUtils.getSourceLabel(url) + ":" + line;

      
      this._showBreakpoint(aBreakpointClient, aFlags);

      
      aCallback && aCallback(aBreakpointClient, aResponse.error);
    }.bind(this));
  },

  











  removeBreakpoint:
  function BP_removeBreakpoint(aBreakpointClient, aCallback, aFlags = {}) {
    let breakpointActor = (aBreakpointClient || {}).actor;

    
    if (!this.store[breakpointActor]) {
      aCallback && aCallback(aBreakpointClient.location);
      return;
    }

    aBreakpointClient.remove(function() {
      
      delete this.store[breakpointActor];

      
      this._hideBreakpoint(aBreakpointClient, aFlags);

      
      aCallback && aCallback(aBreakpointClient.location);
    }.bind(this));
  },

  







  _showBreakpoint: function BP__showBreakpoint(aBreakpointClient, aFlags = {}) {
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
      let { lineText, lineInfo, actor } = aBreakpointClient;
      let conditionalFlag = aBreakpointClient.conditionalExpression !== undefined;
      let openPopupFlag = aFlags.openPopup;

      DebuggerView.Breakpoints.addBreakpoint(
        url, line, actor, lineInfo, lineText, conditionalFlag, openPopupFlag);
    }
    
    if (!aFlags.noPaneHighlight) {
      DebuggerView.Breakpoints.highlightBreakpoint(url, line);
    }
  },

  







  _hideBreakpoint: function BP__hideBreakpoint(aBreakpointClient, aFlags = {}) {
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
      DebuggerView.Breakpoints.removeBreakpoint(url, line);
    }
  },

  









  getBreakpoint: function BP_getBreakpoint(aUrl, aLine) {
    for each (let breakpointClient in this.store) {
      if (breakpointClient.location.url == aUrl &&
          breakpointClient.location.line == aLine) {
        return breakpointClient;
      }
    }
    return null;
  }
};




let L10N = {
  





  getStr: function L10N_getStr(aName) {
    return this.stringBundle.GetStringFromName(aName);
  },

  






  getFormatStr: function L10N_getFormatStr(aName, aArray) {
    return this.stringBundle.formatStringFromName(aName, aArray, aArray.length);
  }
};

XPCOMUtils.defineLazyGetter(L10N, "stringBundle", function() {
  return Services.strings.createBundle(DBG_STRINGS_URI);
});

XPCOMUtils.defineLazyGetter(L10N, "ellipsis", function() {
  return Services.prefs.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString).data;
});




let Prefs = {
  






  _get: function P__get(aType, aPrefName) {
    if (this[aPrefName] === undefined) {
      this[aPrefName] = Services.prefs["get" + aType + "Pref"](aPrefName);
    }
    return this[aPrefName];
  },

  






  _set: function P__set(aType, aPrefName, aValue) {
    Services.prefs["set" + aType + "Pref"](aPrefName, aValue);
    this[aPrefName] = aValue;
  },

  






  map: function P_map(aType, aPropertyName, aPrefName) {
    Object.defineProperty(this, aPropertyName, {
      get: function() this._get(aType, aPrefName),
      set: function(aValue) this._set(aType, aPrefName, aValue)
    });
  }
};

Prefs.map("Int", "windowX", "devtools.debugger.ui.win-x");
Prefs.map("Int", "windowY", "devtools.debugger.ui.win-y");
Prefs.map("Int", "windowWidth", "devtools.debugger.ui.win-width");
Prefs.map("Int", "windowHeight", "devtools.debugger.ui.win-height");
Prefs.map("Int", "stackframesWidth", "devtools.debugger.ui.stackframes-width");
Prefs.map("Int", "variablesWidth", "devtools.debugger.ui.variables-width");
Prefs.map("Bool", "pauseOnExceptions", "devtools.debugger.ui.pause-on-exceptions");
Prefs.map("Bool", "panesVisibleOnStartup", "devtools.debugger.ui.panes-visible-on-startup");
Prefs.map("Bool", "variablesSortingEnabled", "devtools.debugger.ui.variables-sorting-enabled");
Prefs.map("Bool", "variablesOnlyEnumVisible", "devtools.debugger.ui.variables-only-enum-visible");
Prefs.map("Bool", "variablesSearchboxVisible", "devtools.debugger.ui.variables-searchbox-visible");
Prefs.map("Char", "remoteHost", "devtools.debugger.remote-host");
Prefs.map("Int", "remotePort", "devtools.debugger.remote-port");
Prefs.map("Bool", "remoteAutoConnect", "devtools.debugger.remote-autoconnect");
Prefs.map("Int", "remoteConnectionRetries", "devtools.debugger.remote-connection-retries");
Prefs.map("Int", "remoteTimeout", "devtools.debugger.remote-timeout");





XPCOMUtils.defineLazyGetter(window, "_isRemoteDebugger", function() {
  
  return !(window.frameElement instanceof XULElement) &&
         !!window._remoteFlag;
});





XPCOMUtils.defineLazyGetter(window, "_isChromeDebugger", function() {
  
  return !(window.frameElement instanceof XULElement) &&
         !window._remoteFlag;
});




DebuggerController.initialize();
DebuggerController.ThreadState = new ThreadState();
DebuggerController.StackFrames = new StackFrames();
DebuggerController.SourceScripts = new SourceScripts();
DebuggerController.Breakpoints = new Breakpoints();




Object.defineProperties(window, {
  "gClient": {
    get: function() DebuggerController.client
  },
  "gTabClient": {
    get: function() DebuggerController.tabClient
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
  },
  "dispatchEvent": {
    get: function() DebuggerController.dispatchEvent,
  },
  "editor": {
    get: function() DebuggerView.editor
  }
});





function dumpn(str) {
  if (wantLogging) {
    dump("DBG-FRONTEND: " + str + "\n");
  }
}

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");
