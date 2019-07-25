




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const FRAME_STEP_CACHE_DURATION = 100; 
const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const SCRIPTS_URL_MAX_LENGTH = 64; 
const SYNTAX_HIGHLIGHT_MAX_FILE_SIZE = 1048576; 

Cu.import("resource:///modules/source-editor.jsm");
Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import('resource://gre/modules/Services.jsm');





let DebuggerController = {

  


  init: function() {
    this._startupDebugger = this._startupDebugger.bind(this);
    this._shutdownDebugger = this._shutdownDebugger.bind(this);
    this._onTabNavigated = this._onTabNavigated.bind(this);
    this._onTabDetached = this._onTabDetached.bind(this);

    window.addEventListener("DOMContentLoaded", this._startupDebugger, true);
    window.addEventListener("unload", this._shutdownDebugger, true);
  },

  


  _startupDebugger: function DC__startupDebugger() {
    if (this._isInitialized) {
      return;
    }
    this._isInitialized = true;
    window.removeEventListener("DOMContentLoaded", this._startupDebugger, true);

    DebuggerView.initializePanes();
    DebuggerView.initializeEditor(function() {
      DebuggerView.GlobalSearch.initialize();
      DebuggerView.Scripts.initialize();
      DebuggerView.StackFrames.initialize();
      DebuggerView.Breakpoints.initialize();
      DebuggerView.Properties.initialize();
      DebuggerView.showCloseButton(!this._isRemoteDebugger && !this._isChromeDebugger);

      this.dispatchEvent("Debugger:Loaded");
      this._connect();
    }.bind(this));
  },

  



  _shutdownDebugger: function DC__shutdownDebugger() {
    if (this._isDestroyed) {
      return;
    }
    this._isDestroyed = true;
    window.removeEventListener("unload", this._shutdownDebugger, true);

    DebuggerView.GlobalSearch.destroy();
    DebuggerView.Scripts.destroy();
    DebuggerView.StackFrames.destroy();
    DebuggerView.Breakpoints.destroy();
    DebuggerView.Properties.destroy();
    DebuggerView.destroyPanes();
    DebuggerView.destroyEditor();

    DebuggerController.SourceScripts.disconnect();
    DebuggerController.StackFrames.disconnect();
    DebuggerController.ThreadState.disconnect();

    this.dispatchEvent("Debugger:Unloaded");
    this._disconnect();
    this._isChromeDebugger && this._quitApp();
  },

  





  _prepareConnection: function DC__prepareConnection() {
    
    if (this._remoteConnectionTry === Prefs.remoteConnectionRetries) {
      Services.prompt.alert(null,
        L10N.getStr("remoteDebuggerPromptTitle"),
        L10N.getStr("remoteDebuggerConnectionFailedMessage"));
      this.dispatchEvent("Debugger:Close");
      return false;
    }

    
    if (!Prefs.remoteAutoConnect) {
      let prompt = new RemoteDebuggerPrompt();
      let result = prompt.show(!!this._remoteConnectionTimeout);
      
      
      if (!result && !DebuggerController.activeThread) {
        this.dispatchEvent("Debugger:Close");
        return false;
      }
      Prefs.remoteHost = prompt.remote.host;
      Prefs.remotePort = prompt.remote.port;
    }

    
    
    this._remoteConnectionTry = ++this._remoteConnectionTry || 1;
    this._remoteConnectionTimeout = window.setTimeout(function() {
      
      if (!DebuggerController.activeThread) {
        DebuggerController._onRemoteConnectionTimeout();
        DebuggerController._connect();
      }
    }, Prefs.remoteTimeout);

    return true;
  },

  


  _onRemoteConnectionTimeout: function DC__onRemoteConnectionTimeout() {
    Cu.reportError("Couldn't connect to " +
      Prefs.remoteHost + ":" + Prefs.remotePort);
  },

  



  _connect: function DC__connect() {
    if (this._isRemoteDebugger) {
      if (!this._prepareConnection()) {
        return;
      }
    }

    let transport = (this._isChromeDebugger || this._isRemoteDebugger)
      ? debuggerSocketConnect(Prefs.remoteHost, Prefs.remotePort)
      : DebuggerServer.connectPipe();

    let client = this.client = new DebuggerClient(transport);

    client.addListener("tabNavigated", this._onTabNavigated);
    client.addListener("tabDetached", this._onTabDetached);

    client.connect(function(aType, aTraits) {
      client.listTabs(function(aResponse) {
        let tab = aResponse.tabs[aResponse.selected];
        this._startDebuggingTab(client, tab);
        this.dispatchEvent("Debugger:Connecting");
      }.bind(this));
    }.bind(this));
  },

  


  _disconnect: function DC__disconnect() {
    this.client.removeListener("tabNavigated", this._onTabNavigated);
    this.client.removeListener("tabDetached", this._onTabDetached);
    this.client.close();

    this.client = null;
    this.tabClient = null;
    this.activeThread = null;
  },

  


  _onTabNavigated: function DC__onTabNavigated(aNotification, aPacket) {
    DebuggerController.ThreadState._handleTabNavigation(function() {
      DebuggerController.StackFrames._handleTabNavigation(function() {
        DebuggerController.SourceScripts._handleTabNavigation();
      });
    });
  },

  


  _onTabDetached: function DC__onTabDetached() {
    this.dispatchEvent("Debugger:Close");
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

        DebuggerController.ThreadState.connect(function() {
          DebuggerController.StackFrames.connect(function() {
            DebuggerController.SourceScripts.connect(function() {
              aThreadClient.resume();
            });
          });
        });

      }.bind(this));
    }.bind(this));
  },

  



  get _isRemoteDebugger() {
    return window._remoteFlag;
  },

  



  get _isChromeDebugger() {
    
    return !("content" in window.parent) && !this._isRemoteDebugger;
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

  


  get activeThread() {
    return DebuggerController.activeThread;
  },

  





  connect: function TS_connect(aCallback) {
    this.activeThread.addListener("paused", this._update);
    this.activeThread.addListener("resumed", this._update);
    this.activeThread.addListener("detached", this._update);

    this._handleTabNavigation();

    aCallback && aCallback();
  },

  


  disconnect: function TS_disconnect() {
    if (!this.activeThread) {
      return;
    }
    this.activeThread.removeListener("paused", this._update);
    this.activeThread.removeListener("resumed", this._update);
    this.activeThread.removeListener("detached", this._update);
  },

  


  _handleTabNavigation: function TS__handleTabNavigation(aCallback) {
    DebuggerView.StackFrames.updateState(this.activeThread.state);

    aCallback && aCallback();
  },

  


  _update: function TS__update(aEvent) {
    DebuggerView.StackFrames.updateState(this.activeThread.state);
  }
};





function StackFrames() {
  this._onPaused = this._onPaused.bind(this);
  this._onResume = this._onResume.bind(this);
  this._onFrames = this._onFrames.bind(this);
  this._onFramesCleared = this._onFramesCleared.bind(this);
  this._afterFramesCleared = this._afterFramesCleared.bind(this);
}

StackFrames.prototype = {

  


  pageSize: 25,

  


  selectedFrame: null,

  



  pauseOnExceptions: false,

  


  get activeThread() {
    return DebuggerController.activeThread;
  },

  





  connect: function SF_connect(aCallback) {
    window.addEventListener("Debugger:FetchedVariables", this._onFetchedVars, false);
    this.activeThread.addListener("paused", this._onPaused);
    this.activeThread.addListener("resumed", this._onResume);
    this.activeThread.addListener("framesadded", this._onFrames);
    this.activeThread.addListener("framescleared", this._onFramesCleared);

    this._handleTabNavigation();
    this.updatePauseOnExceptions(this.pauseOnExceptions);

    aCallback && aCallback();
  },

  


  disconnect: function SF_disconnect() {
    if (!this.activeThread) {
      return;
    }
    window.removeEventListener("Debugger:FetchedVariables", this._onFetchedVars, false);
    this.activeThread.removeListener("paused", this._onPaused);
    this.activeThread.removeListener("resumed", this._onResume);
    this.activeThread.removeListener("framesadded", this._onFrames);
    this.activeThread.removeListener("framescleared", this._onFramesCleared);
  },

  


  _handleTabNavigation: function SF__handleTabNavigation(aCallback) {
    

    aCallback && aCallback();
  },

  







  _onPaused: function SF__onPaused(aEvent, aPacket) {
    
    if (aPacket.why.type == "exception") {
      this.exception = aPacket.why.exception;
    }

    this.activeThread.fillFrames(this.pageSize);
    DebuggerView.editor.focus();
  },

  


  _onResume: function SF__onResume() {
    DebuggerView.editor.setDebugLocation(-1);
  },

  


  _onFrames: function SF__onFrames() {
    if (!this.activeThread.cachedFrames.length) {
      DebuggerView.StackFrames.emptyText();
      DebuggerView.Properties.emptyText();
      return;
    }
    DebuggerView.StackFrames.empty();
    DebuggerView.Properties.empty();

    for each (let frame in this.activeThread.cachedFrames) {
      this._addFrame(frame);
    }
    if (!this.selectedFrame) {
      this.selectFrame(0);
    }
    if (this.activeThread.moreFrames) {
      DebuggerView.StackFrames.dirty = true;
    }
  },

  


  _onFramesCleared: function SF__onFramesCleared() {
    this.selectedFrame = null;
    this.exception = null;
    
    
    
    
    window.setTimeout(this._afterFramesCleared, FRAME_STEP_CACHE_DURATION);
  },

  


  _afterFramesCleared: function SF__afterFramesCleared() {
    if (!this.activeThread.cachedFrames.length) {
      DebuggerView.StackFrames.emptyText();
      DebuggerView.Properties.emptyText();
      DebuggerController.dispatchEvent("Debugger:AfterFramesCleared");
    }
  },

  



  updateEditorLocation: function SF_updateEditorLocation() {
    let frame = this.activeThread.cachedFrames[this.selectedFrame];
    if (!frame) {
      return;
    }

    let url = frame.where.url;
    let line = frame.where.line;
    let editor = DebuggerView.editor;

    this.updateEditorToLocation(url, line, true);
  },

  














  updateEditorToLocation:
  function SF_updateEditorToLocation(aUrl, aLine, aNoSwitch, aNoCaretFlag, aNoDebugFlag) {
    let editor = DebuggerView.editor;

    function set() {
      if (!aNoCaretFlag) {
        editor.setCaretPosition(aLine - 1);
      }
      if (!aNoDebugFlag) {
        editor.setDebugLocation(aLine - 1);
      }
    }

    
    if (DebuggerView.Scripts.isSelected(aUrl)) {
      return set();
    }
    if (!aNoSwitch && DebuggerView.Scripts.contains(aUrl)) {
      DebuggerView.Scripts.selectScript(aUrl);
      return set();
    }
    editor.setCaretPosition(-1);
    editor.setDebugLocation(-1);
  },

  






  updatePauseOnExceptions: function SF_updatePauseOnExceptions(aFlag) {
    this.pauseOnExceptions = aFlag;
    this.activeThread.pauseOnExceptions(this.pauseOnExceptions);
  },

  






  selectFrame: function SF_selectFrame(aDepth) {
    
    if (this.selectedFrame !== null) {
      DebuggerView.StackFrames.unhighlightFrame(this.selectedFrame);
    }

    
    this.selectedFrame = aDepth;
    DebuggerView.StackFrames.highlightFrame(this.selectedFrame);

    let frame = this.activeThread.cachedFrames[aDepth];
    if (!frame) {
      return;
    }

    let url = frame.where.url;
    let line = frame.where.line;

    
    this.updateEditorToLocation(url, line);

    
    DebuggerView.Properties.createHierarchyStore();

    
    DebuggerView.Properties.empty();

    if (frame.environment) {
      let env = frame.environment;
      do {
        
        let name = env.type.charAt(0).toUpperCase() + env.type.slice(1);
        
        if (!env.parent) {
          name = L10N.getStr("globalScopeLabel");
        }
        let label = L10N.getFormatStr("scopeLabel", [name]);
        switch (env.type) {
          case "with":
          case "object":
            label += " [" + env.object.class + "]";
            break;
          case "function":
            if (env.functionName) {
              label += " [" + env.functionName + "]";
            }
            break;
          default:
            break;
        }

        let scope = DebuggerView.Properties.addScope(label);

        
        if (env == frame.environment) {
          
          if (aDepth == 0 && this.exception) {
            let excVar = scope.addVar("<exception>");
            if (typeof this.exception == "object") {
              excVar.setGrip({
                type: this.exception.type,
                class: this.exception.class
              });
              this._addExpander(excVar, this.exception);
            } else {
              excVar.setGrip(this.exception);
            }
          }

          
          if (frame.this) {
            let thisVar = scope.addVar("this");
            thisVar.setGrip({
              type: frame.this.type,
              class: frame.this.class
            });
            this._addExpander(thisVar, frame.this);
          }

          
          scope.expand(true);
          scope.addToHierarchy();
        }

        switch (env.type) {
          case "with":
          case "object":
            let objClient = this.activeThread.pauseGrip(env.object);
            objClient.getPrototypeAndProperties(function SF_getProps(aResponse) {
              this._addScopeVariables(aResponse.ownProperties, scope);
              
              DebuggerController.dispatchEvent("Debugger:FetchedVariables");
            }.bind(this));
            break;
          case "block":
          case "function":
            
            let variables = env.bindings.arguments;
            for each (let variable in variables) {
              let name = Object.getOwnPropertyNames(variable)[0];
              let paramVar = scope.addVar(name, variable[name]);
              let paramVal = variable[name].value;
              paramVar.setGrip(paramVal);
              this._addExpander(paramVar, paramVal);
            }
            
            this._addScopeVariables(env.bindings.variables, scope);
            break;
          default:
            Cu.reportError("Unknown Debugger.Environment type: " + env.type);
            break;
        }
      } while (env = env.parent);
    }

    
    DebuggerController.dispatchEvent("Debugger:FetchedVariables");
  },

  


  _onFetchedVars: function SF__onFetchedVars() {
    DebuggerView.Properties.commitHierarchy();
  },

  








  _addScopeVariables: function SF_addScopeVariables(aVariables, aScope) {
    
    let variables = {};
    for each (let prop in Object.keys(aVariables).sort()) {
      variables[prop] = aVariables[prop];
    }

    
    for (let variable in variables) {
      let paramVar = aScope.addVar(variable, variables[variable]);
      let paramVal = variables[variable].value;
      paramVar.setGrip(paramVal);
      this._addExpander(paramVar, paramVal);
    }
  },

  



  _addExpander: function SF__addExpander(aVar, aObject) {
    
    if (!aVar || !aObject || typeof aObject !== "object" ||
        aObject.type !== "object") {
      return;
    }

    
    aVar.forceShowArrow();
    aVar.onexpand = this._addVarProperties.bind(this, aVar, aObject);
  },

  



  _addVarProperties: function SF__addVarProperties(aVar, aObject) {
    
    if (aVar.fetched) {
      return;
    }

    let objClient = this.activeThread.pauseGrip(aObject);
    objClient.getPrototypeAndProperties(function SF_onProtoAndProps(aResponse) {
      
      let properties = {};
      for each (let prop in Object.keys(aResponse.ownProperties).sort()) {
        properties[prop] = aResponse.ownProperties[prop];
      }
      aVar.addProperties(properties);

      
      for (let prop in aResponse.ownProperties) {
        this._addExpander(aVar[prop], aResponse.ownProperties[prop].value);
      }

      
      if (aResponse.prototype.type !== "null") {
        let properties = { "__proto__ ": { value: aResponse.prototype } };
        aVar.addProperties(properties);

        
        this._addExpander(aVar["__proto__ "], aResponse.prototype);
      }
      aVar.fetched = true;
    }.bind(this));
  },

  





  _addFrame: function SF__addFrame(aFrame) {
    let depth = aFrame.depth;
    let label = DebuggerController.SourceScripts.getScriptLabel(aFrame.where.url);

    let startText = this._getFrameTitle(aFrame);
    let endText = label + ":" + aFrame.where.line;

    let frame = DebuggerView.StackFrames.addFrame(depth, startText, endText);
    if (frame) {
      frame.debuggerFrame = aFrame;
    }
  },

  


  addMoreFrames: function SF_addMoreFrames() {
    this.activeThread.fillFrames(
      this.activeThread.cachedFrames.length + this.pageSize);
  },

  






  _getFrameTitle: function SF__getFrameTitle(aFrame) {
    if (aFrame.type == "call") {
      return aFrame["calleeName"] ? aFrame["calleeName"] : "(anonymous)";
    }
    return "(" + aFrame.type + ")";
  },

  






  evaluate: function SF_evaluate(aExpression) {
    let frame = this.activeThread.cachedFrames[this.selectedFrame];
    this.activeThread.eval(frame.actor, aExpression);
  }
};





function SourceScripts() {
  this._onNewScript = this._onNewScript.bind(this);
  this._onScriptsAdded = this._onScriptsAdded.bind(this);
  this._onShowScript = this._onShowScript.bind(this);
  this._onLoadSource = this._onLoadSource.bind(this);
  this._onLoadSourceFinished = this._onLoadSourceFinished.bind(this);
}

SourceScripts.prototype = {

  


  _labelsCache: {},

  


  get activeThread() {
    return DebuggerController.activeThread;
  },

  


  get debuggerClient() {
    return DebuggerController.client;
  },

  





  connect: function SS_connect(aCallback) {
    window.addEventListener("Debugger:LoadSource", this._onLoadSource, false);
    this.debuggerClient.addListener("newScript", this._onNewScript);

    this._handleTabNavigation();

    aCallback && aCallback();
  },

  


  disconnect: function SS_disconnect() {
    if (!this.activeThread) {
      return;
    }
    window.removeEventListener("Debugger:LoadSource", this._onLoadSource, false);
    this.debuggerClient.removeListener("newScript", this._onNewScript);
  },

  


  _handleTabNavigation: function SS__handleTabNavigation(aCallback) {
    this._clearLabelsCache();
    this._onScriptsCleared();

    
    
    this.activeThread.getScripts(this._onScriptsAdded);

    aCallback && aCallback();
  },

  


  _onNewScript: function SS__onNewScript(aNotification, aPacket) {
    
    if (aPacket.url == "debugger eval code") {
      return;
    }

    this._addScript({ url: aPacket.url, startLine: aPacket.startLine }, true);

    let preferredScriptUrl = DebuggerView.Scripts.preferredScriptUrl;

    
    if (aPacket.url === DebuggerView.Scripts.preferredScriptUrl) {
      DebuggerView.Scripts.selectScript(aPacket.url);
    }
    
    else if (!DebuggerView.Scripts.selected) {
      DebuggerView.Scripts.selectIndex(0);
    }

    
    
    for each (let breakpoint in DebuggerController.Breakpoints.store) {
      if (breakpoint.location.url == aPacket.url) {
        DebuggerController.Breakpoints.displayBreakpoint(breakpoint);
      }
    }

    DebuggerController.dispatchEvent("Debugger:AfterNewScript");
  },

  


  _onScriptsAdded: function SS__onScriptsAdded(aResponse) {
    for each (let script in aResponse.scripts) {
      this._addScript(script, false);
    }
    DebuggerView.Scripts.commitScripts();
    DebuggerController.Breakpoints.updatePaneBreakpoints();

    let preferredScriptUrl = DebuggerView.Scripts.preferredScriptUrl;

    
    if (preferredScriptUrl && DebuggerView.Scripts.contains(preferredScriptUrl)) {
      DebuggerView.Scripts.selectScript(preferredScriptUrl);
    }
    
    else if (!DebuggerView.Scripts.selected) {
      DebuggerView.Scripts.selectIndex(0);
    }

    DebuggerController.dispatchEvent("Debugger:AfterScriptsAdded");
  },

  


  _onScriptsCleared: function SS__onScriptsCleared() {
    DebuggerView.GlobalSearch.hideAndEmpty();
    DebuggerView.GlobalSearch.clearCache();
    DebuggerView.Scripts.clearSearch();
    DebuggerView.Scripts.empty();
    DebuggerView.Breakpoints.emptyText();
    DebuggerView.editor.setText("");
  },

  








  _setEditorMode: function SS__setEditorMode(aUrl, aContentType) {
    if (aContentType) {
      if (/javascript/.test(aContentType)) {
        DebuggerView.editor.setMode(SourceEditor.MODES.JAVASCRIPT);
      } else {
        DebuggerView.editor.setMode(SourceEditor.MODES.HTML);
      }
      return;
    }

    
    if (/\.jsm?$/.test(this.trimUrlQuery(aUrl))) {
      DebuggerView.editor.setMode(SourceEditor.MODES.JAVASCRIPT);
    } else {
      DebuggerView.editor.setMode(SourceEditor.MODES.HTML);
    }
  },

  







  trimUrlQuery: function SS_trimUrlQuery(aUrl) {
    let length = aUrl.length;
    let q1 = aUrl.indexOf('?');
    let q2 = aUrl.indexOf('&');
    let q3 = aUrl.indexOf('#');
    let q = Math.min(q1 !== -1 ? q1 : length,
                     q2 !== -1 ? q2 : length,
                     q3 !== -1 ? q3 : length);

    return aUrl.slice(0, q);
  },

  










  trimUrlLength: function SS_trimUrlLength(aUrl, aMaxLength = SCRIPTS_URL_MAX_LENGTH) {
    if (aUrl.length > aMaxLength) {
      let ellipsis = Services.prefs.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString);
      return aUrl.substring(0, aMaxLength) + ellipsis.data;
    }
    return aUrl;
  },

  












  _trimUrl: function SS__trimUrl(aUrl, aLabel, aSeq) {
    if (!(aUrl instanceof Ci.nsIURL)) {
      try {
        
        aUrl = Services.io.newURI(aUrl, null, null).QueryInterface(Ci.nsIURL);
      } catch (e) {
        
        return aUrl;
      }
    }
    if (!aSeq) {
      let name = aUrl.fileName;
      if (name) {
        
        

        
        
        aLabel = aUrl.fileName.replace(/\&.*/, "");
      } else {
        
        
        aLabel = "";
      }
      aSeq = 1;
    }

    
    if (aLabel && aLabel.indexOf("?") !== 0) {

      if (DebuggerView.Scripts.containsIgnoringQuery(aUrl.spec)) {
        
        
        return aLabel;
      }
      if (!DebuggerView.Scripts.containsLabel(aLabel)) {
        
        return aLabel;
      }
    }

    
    if (aSeq === 1) {
      let query = aUrl.query;
      if (query) {
        return this._trimUrl(aUrl, aLabel + "?" + query, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq === 2) {
      let ref = aUrl.ref;
      if (ref) {
        return this._trimUrl(aUrl, aLabel + "#" + aUrl.ref, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq === 3) {
      let dir = aUrl.directory;
      if (dir) {
        return this._trimUrl(aUrl, dir.replace(/^\//, "") + aLabel, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq === 4) {
      let host = aUrl.hostPort;
      if (host) {
        return this._trimUrl(aUrl, host + "/" + aLabel, aSeq + 1);
      }
      aSeq++;
    }
    
    if (aSeq === 5) {
      return this._trimUrl(aUrl, aUrl.specIgnoringRef, aSeq + 1);
    }
    
    return aUrl.spec;
  },

  










  getScriptLabel: function SS_getScriptLabel(aUrl, aHref) {
    if (!this._labelsCache[aUrl]) {
      this._labelsCache[aUrl] = this.trimUrlLength(this._trimUrl(aUrl));
    }
    return this._labelsCache[aUrl];
  },

  



  _clearLabelsCache: function SS__clearLabelsCache() {
    this._labelsCache = {};
  },

  







  _addScript: function SS__addScript(aScript, aForceFlag) {
    DebuggerView.Scripts.addScript(
      this.getScriptLabel(aScript.url), aScript, aForceFlag);
  },

  









  showScript: function SS_showScript(aScript, aOptions = {}) {
    if (aScript.loaded) {
      this._onShowScript(aScript, aOptions);
      return;
    }

    let editor = DebuggerView.editor;
    editor.setMode(SourceEditor.MODES.TEXT);
    editor.setText(L10N.getStr("loadingText"));
    editor.resetUndo();

    
    DebuggerController.dispatchEvent("Debugger:LoadSource", {
      url: aScript.url,
      options: aOptions
    });
  },

  









  _onShowScript: function SS__onShowScript(aScript, aOptions = {}) {
    if (aScript.text.length < SYNTAX_HIGHLIGHT_MAX_FILE_SIZE) {
      this._setEditorMode(aScript.url, aScript.contentType);
    }

    let editor = DebuggerView.editor;
    editor.setText(aScript.text);
    editor.resetUndo();

    DebuggerController.Breakpoints.updateEditorBreakpoints();
    DebuggerController.StackFrames.updateEditorLocation();

    
    if (aOptions.targetLine) {
      editor.setCaretPosition(aOptions.targetLine - 1);
    }

    
    DebuggerController.dispatchEvent("Debugger:ScriptShown", {
      url: aScript.url
    });
  },

  







  _onLoadSource: function SS__onLoadSource(aEvent) {
    let url = aEvent.detail.url;
    let options = aEvent.detail.options;
    let self = this;

    switch (Services.io.extractScheme(url)) {
      case "file":
      case "chrome":
      case "resource":
        try {
          NetUtil.asyncFetch(url, function onFetch(aStream, aStatus) {
            if (!Components.isSuccessCode(aStatus)) {
              return self._logError(url, aStatus);
            }
            let source = NetUtil.readInputStreamToString(aStream, aStream.available());
            self._onLoadSourceFinished(url, source, null, options);
            aStream.close();
          });
        } catch (ex) {
          return self._logError(url, ex.name);
        }
        break;

      default:
        let channel = Services.io.newChannel(url, null, null);
        let chunks = [];
        let streamListener = {
          onStartRequest: function(aRequest, aContext, aStatusCode) {
            if (!Components.isSuccessCode(aStatusCode)) {
              return self._logError(url, aStatusCode);
            }
          },
          onDataAvailable: function(aRequest, aContext, aStream, aOffset, aCount) {
            chunks.push(NetUtil.readInputStreamToString(aStream, aCount));
          },
          onStopRequest: function(aRequest, aContext, aStatusCode) {
            if (!Components.isSuccessCode(aStatusCode)) {
              return self._logError(url, aStatusCode);
            }
            self._onLoadSourceFinished(
              url, chunks.join(""), channel.contentType, options);
          }
        };

        channel.loadFlags = channel.LOAD_FROM_CACHE;
        channel.asyncOpen(streamListener, null);
        break;
    }
  },

  













  _onLoadSourceFinished:
  function SS__onLoadSourceFinished(aScriptUrl, aSourceText, aContentType, aOptions) {
    let element = DebuggerView.Scripts.getScriptByLocation(aScriptUrl);
    let script = element.getUserData("sourceScript");

    script.loaded = true;
    script.text = aSourceText;
    script.contentType = aContentType;
    element.setUserData("sourceScript", script, null);

    if (aOptions.silent) {
      aOptions.callback && aOptions.callback(aScriptUrl, aSourceText);
      return;
    }

    this.showScript(script, aOptions);
  },

  








  getLineText: function SS_getLineText(aLine) {
    let editor = DebuggerView.editor;
    let line = aLine || editor.getCaretPosition().line;
    let start = editor.getLineStart(line);
    let end = editor.getLineEnd(line);
    return editor.getText(start, end);
  },

  







  _logError: function SS__logError(aUrl, aStatus) {
    Cu.reportError(L10N.getFormatStr("loadingError", [aUrl, aStatus]));
  },
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

  














  _skipEditorBreakpointChange: false,

  







  store: {},

  


  get activeThread() {
    return DebuggerController.ThreadState.activeThread;
  },

  


  get editor() {
    return DebuggerView.editor;
  },

  


  initialize: function BP_initialize() {
    this.editor.addEventListener(
      SourceEditor.EVENTS.BREAKPOINT_CHANGE, this._onEditorBreakpointChange);
  },

  


  destroy: function BP_destroy() {
    for each (let breakpoint in this.store) {
      this.removeBreakpoint(breakpoint);
    }
  },

  








  _onEditorBreakpointChange: function BP__onEditorBreakpointChange(aEvent) {
    if (this._skipEditorBreakpointChange) {
      return;
    }

    aEvent.added.forEach(this._onEditorBreakpointAdd, this);
    aEvent.removed.forEach(this._onEditorBreakpointRemove, this);
  },

  






  _onEditorBreakpointAdd: function BP__onEditorBreakpointAdd(aBreakpoint) {
    let url = DebuggerView.Scripts.selected;
    if (!url) {
      return;
    }

    let line = aBreakpoint.line + 1;

    this.addBreakpoint({ url: url, line: line }, null, true);
  },

  






  _onEditorBreakpointRemove: function BP__onEditorBreakpointRemove(aBreakpoint) {
    let url = DebuggerView.Scripts.selected;
    if (!url) {
      return;
    }

    let line = aBreakpoint.line + 1;

    let breakpoint = this.getBreakpoint(url, line);
    if (breakpoint) {
      this.removeBreakpoint(breakpoint, null, true);
    }
  },

  




  updateEditorBreakpoints: function BP_updateEditorBreakpoints() {
    let url = DebuggerView.Scripts.selected;
    if (!url) {
      return;
    }

    this._skipEditorBreakpointChange = true;
    for each (let breakpoint in this.store) {
      if (breakpoint.location.url == url) {
        this.editor.addBreakpoint(breakpoint.location.line - 1);
      }
    }
    this._skipEditorBreakpointChange = false;
  },

  



  updatePaneBreakpoints: function BP_updatePaneBreakpoints() {
    let url = DebuggerView.Scripts.selected;
    if (!url) {
      return;
    }

    this._skipEditorBreakpointChange = true;
    for each (let breakpoint in this.store) {
      if (DebuggerView.Scripts.contains(breakpoint.location.url)) {
        this.displayBreakpoint(breakpoint, true);
      }
    }
    this._skipEditorBreakpointChange = false;
  },

  



















  addBreakpoint:
  function BP_addBreakpoint(aLocation, aCallback, aNoEditorUpdate, aNoPaneUpdate) {
    let breakpoint = this.getBreakpoint(aLocation.url, aLocation.line);
    if (breakpoint) {
      aCallback && aCallback(breakpoint);
      return;
    }

    this.activeThread.setBreakpoint(aLocation, function(aResponse, aBpClient) {
      this.store[aBpClient.actor] = aBpClient;
      this.displayBreakpoint(aBpClient, aNoEditorUpdate, aNoPaneUpdate);
      aCallback && aCallback(aBpClient, aResponse.error);
    }.bind(this));
  },

  










  displayBreakpoint:
  function BP_displayBreakpoint(aBreakpoint, aNoEditorUpdate, aNoPaneUpdate) {
    if (!aNoEditorUpdate) {
      let url = DebuggerView.Scripts.selected;
      if (url == aBreakpoint.location.url) {
        this._skipEditorBreakpointChange = true;
        this.editor.addBreakpoint(aBreakpoint.location.line - 1);
        this._skipEditorBreakpointChange = false;
      }
    }
    if (!aNoPaneUpdate) {
      let { url: url, line: line } = aBreakpoint.location;

      if (!aBreakpoint.lineText || !aBreakpoint.lineInfo) {
        let scripts = DebuggerController.SourceScripts;
        aBreakpoint.lineText = scripts.getLineText(line - 1);
        aBreakpoint.lineInfo = scripts.getScriptLabel(url) + ":" + line;
      }
      DebuggerView.Breakpoints.addBreakpoint(
        aBreakpoint.actor,
        aBreakpoint.lineInfo,
        aBreakpoint.lineText, url, line);
    }
  },

  














  removeBreakpoint:
  function BP_removeBreakpoint(aBreakpoint, aCallback, aNoEditorUpdate, aNoPaneUpdate) {
    if (!(aBreakpoint.actor in this.store)) {
      aCallback && aCallback(aBreakpoint.location);
      return;
    }

    aBreakpoint.remove(function() {
      delete this.store[aBreakpoint.actor];

      if (!aNoEditorUpdate) {
        let url = DebuggerView.Scripts.selected;
        if (url == aBreakpoint.location.url) {
          this._skipEditorBreakpointChange = true;
          this.editor.removeBreakpoint(aBreakpoint.location.line - 1);
          this._skipEditorBreakpointChange = false;
        }
      }
      if (!aNoPaneUpdate) {
        DebuggerView.Breakpoints.removeBreakpoint(aBreakpoint.actor);
      }

      aCallback && aCallback(aBreakpoint.location);
    }.bind(this));
  },

  









  getBreakpoint: function BP_getBreakpoint(aUrl, aLine) {
    for each (let breakpoint in this.store) {
      if (breakpoint.location.url == aUrl && breakpoint.location.line == aLine) {
        return breakpoint;
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




let Prefs = {

  



  get stackframesWidth() {
    if (this._sfrmWidth === undefined) {
      this._sfrmWidth = Services.prefs.getIntPref("devtools.debugger.ui.stackframes-width");
    }
    return this._sfrmWidth;
  },

  



  set stackframesWidth(value) {
    Services.prefs.setIntPref("devtools.debugger.ui.stackframes-width", value);
    this._sfrmWidth = value;
  },

  



  get variablesWidth() {
    if (this._varsWidth === undefined) {
      this._varsWidth = Services.prefs.getIntPref("devtools.debugger.ui.variables-width");
    }
    return this._varsWidth;
  },

  



  set variablesWidth(value) {
    Services.prefs.setIntPref("devtools.debugger.ui.variables-width", value);
    this._varsWidth = value;
  },

  




  get remoteAutoConnect() {
    if (this._autoConn === undefined) {
      this._autoConn = Services.prefs.getBoolPref("devtools.debugger.remote-autoconnect");
    }
    return this._autoConn;
  },

  



  set remoteAutoConnect(value) {
    Services.prefs.setBoolPref("devtools.debugger.remote-autoconnect", value);
    this._autoConn = value;
  }
};





XPCOMUtils.defineLazyGetter(Prefs, "remoteHost", function() {
  return Services.prefs.getCharPref("devtools.debugger.remote-host");
});





XPCOMUtils.defineLazyGetter(Prefs, "remotePort", function() {
  return Services.prefs.getIntPref("devtools.debugger.remote-port");
});





XPCOMUtils.defineLazyGetter(Prefs, "remoteConnectionRetries", function() {
  return Services.prefs.getIntPref("devtools.debugger.remote-connection-retries");
});





XPCOMUtils.defineLazyGetter(Prefs, "remoteTimeout", function() {
  return Services.prefs.getIntPref("devtools.debugger.remote-timeout");
});




DebuggerController.init();
DebuggerController.ThreadState = new ThreadState();
DebuggerController.StackFrames = new StackFrames();
DebuggerController.SourceScripts = new SourceScripts();
DebuggerController.Breakpoints = new Breakpoints();




Object.defineProperty(window, "gClient", {
  get: function() { return DebuggerController.client; }
});

Object.defineProperty(window, "gTabClient", {
  get: function() { return DebuggerController.tabClient; }
});

Object.defineProperty(window, "gThreadClient", {
  get: function() { return DebuggerController.activeThread; }
});
