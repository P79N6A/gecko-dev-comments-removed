




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const NEW_SCRIPT_DISPLAY_DELAY = 200; 
const FRAME_STEP_CLEAR_DELAY = 100; 
const CALL_STACK_PAGE_SIZE = 25; 

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
Cu.import("resource:///modules/source-editor.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");
Cu.import("resource:///modules/devtools/VariablesView.jsm");




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
    if (window._isRemoteDebugger && !this._prepareConnection()) {
      return;
    }
    let transport = (window._isChromeDebugger || window._isRemoteDebugger)
      ? debuggerSocketConnect(Prefs.remoteHost, Prefs.remotePort)
      : DebuggerServer.connectPipe();

    let client = this.client = new DebuggerClient(transport);
    client.addListener("tabNavigated", this._onTabNavigated);
    client.addListener("tabDetached", this._onTabDetached);

    client.connect(function(aType, aTraits) {
      client.listTabs(function(aResponse) {
        let tab = aResponse.tabs[aResponse.selected];
        this._startDebuggingTab(client, tab);
        window.dispatchEvent("Debugger:Connected");
      }.bind(this));
    }.bind(this));
  },

  


  _disconnect: function DC__disconnect() {
    
    if (!this.client) {
      return;
    }
    this.client.removeListener("tabNavigated", this._onTabNavigated);
    this.client.removeListener("tabDetached", this._onTabDetached);
    this.client.close();

    this.client = null;
    this.tabClient = null;
    this.activeThread = null;
  },

  


  _onTabNavigated: function DC__onTabNavigated() {
    DebuggerView._handleTabNavigation();
    this.ThreadState._handleTabNavigation();
    this.StackFrames._handleTabNavigation();
    this.SourceScripts._handleTabNavigation();
  },

  


  _onTabDetached: function DC__onTabDetached() {
    this._shutdownDebugger();
  },

  







  _startDebuggingTab: function DC__startDebuggingTab(aClient, aTabGrip) {
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

      }.bind(this));
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
  currentFrame: null,
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
    
    if (aPacket.why.type == "exception") {
      this.currentException = aPacket.why.exception;
    }

    this.activeThread.fillFrames(CALL_STACK_PAGE_SIZE);
    DebuggerView.editor.focus();
  },

  


  _onResumed: function SF__onResumed() {
    DebuggerView.editor.setDebugLocation(-1);
  },

  


  _onFrames: function SF__onFrames() {
    
    if (!this.activeThread.cachedFrames.length) {
      return;
    }
    DebuggerView.StackFrames.empty();

    for (let frame of this.activeThread.cachedFrames) {
      this._addFrame(frame);
    }
    if (!this.currentFrame) {
      this.selectFrame(0);
    }
    if (this.activeThread.moreFrames) {
      DebuggerView.StackFrames.dirty = true;
    }
  },

  


  _onFramesCleared: function SF__onFramesCleared() {
    this.currentFrame = null;
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
    window.dispatchEvent("Debugger:AfterFramesCleared");
  },

  






  selectFrame: function SF_selectFrame(aDepth) {
    let frame = this.activeThread.cachedFrames[this.currentFrame = aDepth];
    if (!frame) {
      return;
    }
    let env = frame.environment;
    let { url, line } = frame.where;

    
    if (!env) {
      return;
    }

    
    DebuggerView.updateEditor(url, line);
    
    DebuggerView.StackFrames.highlightFrame(aDepth);
    
    DebuggerView.Breakpoints.highlightBreakpoint(url, line);
    
    DebuggerView.Variables.createHierarchy();
    
    DebuggerView.Variables.empty();

    let self = this;
    let name = "";

    do {
      
      if (!env.parent) {
        name = L10N.getStr("globalScopeLabel");
      }
      
      else {
        name = env.type.charAt(0).toUpperCase() + env.type.slice(1);
      }

      let label = L10N.getFormatStr("scopeLabel", [name]);
      switch (env.type) {
        case "with":
        case "object":
          label += " [" + env.object.class + "]";
          break;
        case "function":
          label += " [" + env.functionName + "]";
          break;
      }

      
      let scope = DebuggerView.Variables.addScope(label);

      
      if (env == frame.environment) {
        
        if (aDepth == 0 && this.currentException) {
          let excVar = scope.addVar("<exception>", { value: this.currentException });
          this._addExpander(excVar, this.currentException);
        }
        
        if (frame.this) {
          let thisVar = scope.addVar("this", { value: frame.this });
          this._addExpander(thisVar, frame.this);
        }
        
        scope.expand(true);
      }

      switch (env.type) {
        case "with":
        case "object":
          
          this.activeThread.pauseGrip(env.object).getPrototypeAndProperties(function(aResponse) {
            self._addScopeVariables(aResponse.ownProperties, scope);

            
            window.dispatchEvent("Debugger:FetchedVariables");
            DebuggerView.Variables.commitHierarchy();
          });
          break;
        case "block":
        case "function":
          
          for (let variable of env.bindings.arguments) {
            let name = Object.getOwnPropertyNames(variable)[0];
            let paramVar = scope.addVar(name, variable[name]);
            let paramVal = variable[name].value;
            this._addExpander(paramVar, paramVal);
          }
          
          this._addScopeVariables(env.bindings.variables, scope);
          break;
        default:
          Cu.reportError("Unknown Debugger.Environment type: " + env.type);
          break;
      }
    } while (env = env.parent);

    
    window.dispatchEvent("Debugger:FetchedVariables");
    DebuggerView.Variables.commitHierarchy();
  },

  








  _addScopeVariables: function SF_addScopeVariables(aVariables, aScope) {
    if (!aVariables) {
      return;
    }
    
    let sortedVariableNames = Object.keys(aVariables).sort();

    
    for (let name of sortedVariableNames) {
      let paramVar = aScope.addVar(name, aVariables[name]);
      let paramVal = aVariables[name].value;
      this._addExpander(paramVar, paramVal);
    }
  },

  








  _addExpander: function SF__addExpander(aVar, aGrip) {
    
    if (VariablesView.isPrimitive({ value: aGrip })) {
      return;
    }
    aVar.onexpand = this._addVarProperties.bind(this, aVar, aGrip);
  },

  








  _addVarProperties: function SF__addVarProperties(aVar, aGrip) {
    
    if (aVar.fetched) {
      return;
    }

    this.activeThread.pauseGrip(aGrip).getPrototypeAndProperties(function(aResponse) {
      let { ownProperties, prototype } = aResponse;

      
      if (ownProperties) {
        aVar.addProperties(ownProperties);
        
        for (let name in ownProperties) {
          this._addExpander(aVar.get(name), ownProperties[name].value);
        }
      }

      
      if (prototype.type != "null") {
        aVar.addProperties({ "__proto__ ": { value: prototype } });
        
        this._addExpander(aVar.get("__proto__ "), prototype);
      }

      aVar.fetched = true;

      
      window.dispatchEvent("Debugger:FetchedProperties");
      DebuggerView.Variables.commitHierarchy();
    }.bind(this));
  },

  





  _addFrame: function SF__addFrame(aFrame) {
    let depth = aFrame.depth;
    let { url, line } = aFrame.where;

    let startText = StackFrameUtils.getFrameTitle(aFrame);
    let endText = SourceUtils.getSourceLabel(url) + ":" + line;

    DebuggerView.StackFrames.addFrame(startText, endText, depth, {
      attachment: aFrame
    });
  },

  


  addMoreFrames: function SF_addMoreFrames() {
    this.activeThread.fillFrames(
      this.activeThread.cachedFrames.length + CALL_STACK_PAGE_SIZE);
  },

  






  evaluate: function SF_evaluate(aExpression) {
    let frame = this.activeThread.cachedFrames[this.currentFrame];
    this.activeThread.eval(frame.actor, aExpression);
  }
};









function SourceScripts() {
  this._onNewScript = this._onNewScript.bind(this);
  this._onScriptsAdded = this._onScriptsAdded.bind(this);
}

SourceScripts.prototype = {
  get activeThread() DebuggerController.activeThread,
  get debuggerClient() DebuggerController.client,

  


  connect: function SS_connect() {
    this.debuggerClient.addListener("newScript", this._onNewScript);
    this._handleTabNavigation();
  },

  


  disconnect: function SS_disconnect() {
    if (!this.activeThread) {
      return;
    }
    this.debuggerClient.removeListener("newScript", this._onNewScript);
  },

  


  _handleTabNavigation: function SS__handleTabNavigation() {
    if (!this.activeThread) {
      return;
    }
    dumpn("Handling tab navigation in the SourceScripts");

    
    
    this.activeThread.getScripts(this._onScriptsAdded);
  },

  


  _onNewScript: function SS__onNewScript(aNotification, aPacket) {
    
    if (aPacket.url == "debugger eval code") {
      return;
    }

    
    this._addSource({
      url: aPacket.url,
      startLine: aPacket.startLine,
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
      window.setTimeout(function() {
        
        
        if (!container.selectedValue) {
          container.selectedIndex = 0;
        }
      }, NEW_SCRIPT_DISPLAY_DELAY);
    }

    
    
    DebuggerController.Breakpoints.updateEditorBreakpoints();
    DebuggerController.Breakpoints.updatePaneBreakpoints();

    
    window.dispatchEvent("Debugger:AfterNewScript");
  },

  


  _onScriptsAdded: function SS__onScriptsAdded(aResponse) {
    
    for (let script of aResponse.scripts) {
      this._addSource(script);
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

    
    window.dispatchEvent("Debugger:AfterScriptsAdded");
  },

  








  _addSource: function SS__addSource(aSource, aOptions = {}) {
    let url = aSource.url;
    let label = SourceUtils.getSourceLabel(url);

    DebuggerView.Sources.push(label, url, {
      forced: aOptions.forced,
      attachment: aSource
    });
  },

  







  getText: function SS_getText(aSource, aCallback) {
    
    if (aSource.loaded) {
      aCallback(aSource.url, aSource.text);
      return;
    }

    
    this.activeThread.source(aSource.source).source(function(aResponse) {
      if (aResponse.error) {
        Cu.reportError("Error loading " + aUrl);
        return;
      }
      aCallback(aSource.url, aResponse.source);
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
        this._showBreakpoint(breakpointClient, { noPaneUpdate: true });
      }
    }
  },

  




  updatePaneBreakpoints: function BP_updatePaneBreakpoints() {
    for each (let breakpointClient in this.store) {
      if (DebuggerView.Sources.containsValue(breakpointClient.location.url)) {
        this._showBreakpoint(breakpointClient, { noEditorUpdate: true });
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
        aBreakpointClient.remove();
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
      let { lineText, lineInfo } = aBreakpointClient;
      let actor = aBreakpointClient.actor;
      DebuggerView.Breakpoints.addBreakpoint(lineInfo, lineText, url, line, actor);
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

const STACKFRAMES_WIDTH = "devtools.debugger.ui.stackframes-width";
const VARIABLES_WIDTH = "devtools.debugger.ui.variables-width";
const PANES_VISIBLE_ON_STARTUP = "devtools.debugger.ui.panes-visible-on-startup";
const NON_ENUM_VISIBLE = "devtools.debugger.ui.non-enum-visible";
const REMOTE_AUTO_CONNECT = "devtools.debugger.remote-autoconnect";
const REMOTE_HOST = "devtools.debugger.remote-host";
const REMOTE_PORT = "devtools.debugger.remote-port";
const REMOTE_CONNECTION_RETRIES = "devtools.debugger.remote-connection-retries";
const REMOTE_TIMEOUT = "devtools.debugger.remote-timeout";




let Prefs = {
  



  get stackframesWidth() {
    if (this._stackframesWidth === undefined) {
      this._stackframesWidth = Services.prefs.getIntPref(STACKFRAMES_WIDTH);
    }
    return this._stackframesWidth;
  },

  



  set stackframesWidth(value) {
    Services.prefs.setIntPref(STACKFRAMES_WIDTH, value);
    this._stackframesWidth = value;
  },

  



  get variablesWidth() {
    if (this._variablesWidth === undefined) {
      this._variablesWidth = Services.prefs.getIntPref(VARIABLES_WIDTH);
    }
    return this._variablesWidth;
  },

  



  set variablesWidth(value) {
    Services.prefs.setIntPref(VARIABLES_WIDTH, value);
    this._variablesWidth = value;
  },

  




  get remoteAutoConnect() {
    if (this._autoConnect === undefined) {
      this._autoConnect = Services.prefs.getBoolPref(REMOTE_AUTO_CONNECT);
    }
    return this._autoConnect;
  },

  




  set remoteAutoConnect(value) {
    Services.prefs.setBoolPref(REMOTE_AUTO_CONNECT, value);
    this._autoConnect = value;
  },

  



  get panesVisibleOnStartup() {
    if (this._panesVisible === undefined) {
      this._panesVisible = Services.prefs.getBoolPref(PANES_VISIBLE_ON_STARTUP);
    }
    return this._panesVisible;
  },

  



  set panesVisibleOnStartup(value) {
    Services.prefs.setBoolPref(PANES_VISIBLE_ON_STARTUP, value);
    this._panesVisible = value;
  },

  




  get nonEnumVisible() {
    if (this._nonEnumVisible === undefined) {
      this._nonEnumVisible = Services.prefs.getBoolPref(NON_ENUM_VISIBLE);
    }
    return this._nonEnumVisible;
  },

  




  set nonEnumVisible(value) {
    Services.prefs.setBoolPref(NON_ENUM_VISIBLE, value);
    this._nonEnumVisible = value;
  }
};





XPCOMUtils.defineLazyGetter(Prefs, "remoteHost", function() {
  return Services.prefs.getCharPref(REMOTE_HOST);
});





XPCOMUtils.defineLazyGetter(Prefs, "remotePort", function() {
  return Services.prefs.getIntPref(REMOTE_PORT);
});





XPCOMUtils.defineLazyGetter(Prefs, "remoteConnectionRetries", function() {
  return Services.prefs.getIntPref(REMOTE_CONNECTION_RETRIES);
});





XPCOMUtils.defineLazyGetter(Prefs, "remoteTimeout", function() {
  return Services.prefs.getIntPref(REMOTE_TIMEOUT);
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
