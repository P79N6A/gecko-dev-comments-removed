




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
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
let promise = Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js").Promise;
Cu.import("resource:///modules/source-editor.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");
Cu.import("resource:///modules/devtools/BreadcrumbsWidget.jsm");
Cu.import("resource:///modules/devtools/SideMenuWidget.jsm");
Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/VariablesViewController.jsm");
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

    
    
    if (window._isChromeDebugger) {
      window.removeEventListener("DOMContentLoaded", this.startupDebugger, true);
    }

    let deferred = this._startup = promise.defer();

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

    
    
    if (window._isChromeDebugger) {
      window.removeEventListener("unload", this.shutdownDebugger, true);
    }

    let deferred = this._shutdown = promise.defer();

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

    let deferred = this._connection = promise.defer();

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

    
    let transport = debuggerSocketConnect(
      Prefs.chromeDebuggingHost, Prefs.chromeDebuggingPort);

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

  


  _ensureResumptionOrder: function(aResponse) {
    if (aResponse.error == "wrongOrder") {
      DebuggerView.Toolbar.showResumeWarning(aResponse.lastPausedUrl);
    }
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
    this.client.reconfigureThread({ useSourceMaps: aUseSourceMaps },
                                  (aResponse) => {
      if (aResponse.error) {
        let msg = "Couldn't reconfigure thread: " + aResponse.message;
        Cu.reportError(msg);
        dumpn(msg);
        return;
      }

      DebuggerView._handleTabNavigation();
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
  this._afterFramesCleared = this._afterFramesCleared.bind(this);
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
        DebuggerView.WatchExpressions.removeAt(0);
        DebuggerController.StackFrames.syncWatchExpressions();
        return;
      }
    }


    
    DebuggerView.showInstrumentsPane();

    
    DebuggerView.StackFrames.empty();

    for (let frame of this.activeThread.cachedFrames) {
      let { depth, where: { url, line } } = frame;
      let frameLocation = NetworkHelper.convertToUnicode(unescape(url));
      let frameTitle = StackFrameUtils.getFrameTitle(frame);

      DebuggerView.StackFrames.addFrame(frameTitle, frameLocation, line, depth);
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

    
    let { environment, where: { url, line } } = frame;
    if (!environment) {
      return;
    }

    
    DebuggerView.updateEditor(url, line);
    
    DebuggerView.Sources.highlightBreakpoint(url, line);
    
    DebuggerView.WatchExpressions.toggleContents(false);
    
    DebuggerView.Variables.createHierarchy();
    
    DebuggerView.Variables.empty();


    
    
    if (this.syncedWatchExpressions && aDepth == 0) {
      let label = L10N.getStr("watchExpressionsScopeLabel");
      let scope = DebuggerView.Variables.addScope(label);

      
      scope.descriptorTooltip = false;
      scope.contextMenuId = "debuggerWatchExpressionsContextMenu";
      scope.separatorStr = L10N.getStr("watchExpressionsSeparatorLabel");
      scope.switch = DebuggerView.WatchExpressions.switchExpression;
      scope.delete = DebuggerView.WatchExpressions.deleteExpression;

      
      this._fetchWatchExpressions(scope, this.currentEvaluation.return);

      
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

      
      
      
      if (innermost || this.autoScopeExpand) {
        scope.expand();
      }
    } while ((environment = environment.parent));

    
    window.dispatchEvent(document, "Debugger:FetchedVariables");
    DebuggerView.Variables.commitHierarchy();
  },

  


  addMoreFrames: function() {
    this.activeThread.fillFrames(
      this.activeThread.cachedFrames.length + CALL_STACK_PAGE_SIZE);
  },

  








  evaluate: function(aExpression, aFrame = this.currentFrame || 0) {
    let frame = this.activeThread.cachedFrames[aFrame];
    this.activeThread.eval(frame.actor, aExpression);
  },

  







  _insertScopeFrameReferences: function(aScope, aFrame) {
    
    if (this.currentException) {
      let excRef = aScope.addItem("<exception>", { value: this.currentException });
      DebuggerView.Variables.controller.addExpander(excRef, this.currentException);
    }
    
    if (this.currentReturnedValue) {
      let retRef = aScope.addItem("<return>", { value: this.currentReturnedValue });
      DebuggerView.Variables.controller.addExpander(retRef, this.currentReturnedValue);
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

    
    this.activeThread.pauseGrip(aExp).getPrototypeAndProperties((aResponse) => {
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

      
      window.dispatchEvent(document, "Debugger:FetchedWatchExpressions");
      DebuggerView.Variables.commitHierarchy();
    });
  },

  



  syncWatchExpressions: function() {
    let list = DebuggerView.WatchExpressions.getAllStrings();

    
    
    
    
    let sanitizedExpressions = list.map((aString) => {
      
      try {
        Parser.reflectionAPI.parse(aString);
        return aString; 
      } catch (e) {
        return "\"" + e.name + ": " + e.message + "\""; 
      }
    });

    if (sanitizedExpressions.length) {
      this.syncedWatchExpressions =
        this.currentWatchExpressions =
          "[" +
            sanitizedExpressions.map((aString) =>
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
      this.syncedWatchExpressions =
        this.currentWatchExpressions = null;
    }
    this.currentFrame = null;
    this._onFrames();
  }
};





function SourceScripts() {
  this._onNewGlobal = this._onNewGlobal.bind(this);
  this._onNewSource = this._onNewSource.bind(this);
  this._onSourcesAdded = this._onSourcesAdded.bind(this);
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
      this._newSourceTimeout = window.setTimeout(() => {
        
        
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
      Cu.reportError("Error getting sources: " + aResponse.message);
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

  














  getTextForSource: function(aSource, aOnTimeout, aDelay = FETCH_SOURCE_RESPONSE_DELAY) {
    
    if (aSource._fetched) {
      return aSource._fetched;
    }

    let deferred = promise.defer();
    aSource._fetched = deferred.promise;

    
    if (aOnTimeout) {
      var fetchTimeout = window.setTimeout(() => aOnTimeout(aSource), aDelay);
    }

    
    this.activeThread.source(aSource).source((aResponse) => {
      if (aOnTimeout) {
        window.clearTimeout(fetchTimeout);
      }
      if (aResponse.error) {
        deferred.reject([aSource, aResponse.message]);
      } else {
        deferred.resolve([aSource, aResponse.source]);
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
      let sourceClient = sourceItem.attachment.source;
      this.getTextForSource(sourceClient, onTimeout).then(onFetch, onError);
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
    
    if (!aLocation) {
      aCallback && aCallback(null, new Error("Invalid breakpoint location."));
      return;
    }
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

      
      
      
      aBreakpointClient.lineText = DebuggerView.getEditorLineText(line - 1).trim();

      
      this._showBreakpoint(aBreakpointClient, aFlags);

      
      aCallback && aCallback(aBreakpointClient, aResponse.error);
    });
  },

  











  removeBreakpoint: function(aBreakpointClient, aCallback, aFlags = {}) {
    
    if (!aBreakpointClient) {
      aCallback && aCallback(null, new Error("Invalid breakpoint client."));
      return;
    }
    let breakpointActor = aBreakpointClient.actor;

    
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

    
    window.dispatchEvent(document, "Debugger:BreakpointShown", aBreakpointClient);
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

    
    window.dispatchEvent(document, "Debugger:BreakpointHidden", aBreakpointClient);
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
  sourcesWidth: ["Int", "ui.panes-sources-width"],
  instrumentsWidth: ["Int", "ui.panes-instruments-width"],
  panesVisibleOnStartup: ["Bool", "ui.panes-visible-on-startup"],
  variablesSortingEnabled: ["Bool", "ui.variables-sorting-enabled"],
  variablesOnlyEnumVisible: ["Bool", "ui.variables-only-enum-visible"],
  variablesSearchboxVisible: ["Bool", "ui.variables-searchbox-visible"],
  pauseOnExceptions: ["Bool", "pause-on-exceptions"],
  sourceMapsEnabled: ["Bool", "source-maps-enabled"]
});





XPCOMUtils.defineLazyGetter(window, "_isChromeDebugger", function() {
  
  return !(window.frameElement instanceof XULElement);
});




DebuggerController.initialize();
DebuggerController.Parser = new Parser();
DebuggerController.ThreadState = new ThreadState();
DebuggerController.StackFrames = new StackFrames();
DebuggerController.SourceScripts = new SourceScripts();
DebuggerController.Breakpoints = new Breakpoints();




Object.defineProperties(window, {
  "dispatchEvent": {
    get: function() ViewHelpers.dispatchEvent,
  },
  "editor": {
    get: function() DebuggerView.editor
  },
  "gTarget": {
    get: function() DebuggerController._target
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
