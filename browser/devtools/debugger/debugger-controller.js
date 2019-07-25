








































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";

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

    DebuggerView.initializeEditor();
    DebuggerView.StackFrames.initialize();
    DebuggerView.Properties.initialize();
    DebuggerView.Scripts.initialize();

    this.dispatchEvent("Debugger:Loaded");
    this._connect();
  },

  



  _shutdownDebugger: function DC__shutdownDebugger() {
    if (this._isDestroyed) {
      return;
    }
    this._isDestroyed = true;
    window.removeEventListener("unload", this._shutdownDebugger, true);

    DebuggerView.destroyEditor();
    DebuggerView.Scripts.destroy();
    DebuggerView.StackFrames.destroy();
    DebuggerView.Properties.destroy();

    DebuggerController.SourceScripts.disconnect();
    DebuggerController.StackFrames.disconnect();
    DebuggerController.ThreadState.disconnect();

    this.dispatchEvent("Debugger:Unloaded");
    this._disconnect();
  },

  



  _connect: function DC__connect() {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
    }

    let transport = DebuggerServer.connectPipe();
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
    let client = this.client;

    client.activeThread.detach(function() {
      client.activeTab.detach(function() {
        client.listTabs(function(aResponse) {
          let tab = aResponse.tabs[aResponse.selected];
          this._startDebuggingTab(client, tab);
          this.dispatchEvent("Debugger:Connecting");
        }.bind(this));
      }.bind(this));
    }.bind(this));
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

    this._update();

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

  


  _update: function TS__update(aEvent) {
    DebuggerView.StackFrames.updateState(this.activeThread.state);
  }
};





function StackFrames() {
  this._onPaused = this._onPaused.bind(this);
  this._onResume = this._onResume.bind(this);
  this._onFrames = this._onFrames.bind(this);
  this._onFramesCleared = this._onFramesCleared.bind(this);
}

StackFrames.prototype = {

  


  pageSize: 25,

  


  selectedFrame: null,

  


  get activeThread() {
    return DebuggerController.activeThread;
  },

  





  connect: function SF_connect(aCallback) {
    this.activeThread.addListener("paused", this._onPaused);
    this.activeThread.addListener("resumed", this._onResume);
    this.activeThread.addListener("framesadded", this._onFrames);
    this.activeThread.addListener("framescleared", this._onFramesCleared);

    this._onFramesCleared();

    aCallback && aCallback();
  },

  


  disconnect: function SF_disconnect() {
    if (!this.activeThread) {
      return;
    }
    this.activeThread.removeListener("paused", this._onPaused);
    this.activeThread.removeListener("resumed", this._onResume);
    this.activeThread.removeListener("framesadded", this._onFrames);
    this.activeThread.removeListener("framescleared", this._onFramesCleared);
  },

  


  _onPaused: function SF__onPaused() {
    this.activeThread.fillFrames(this.pageSize);
  },

  


  _onResume: function SF__onResume() {
    DebuggerView.editor.setDebugLocation(-1);
  },

  


  _onFrames: function SF__onFrames() {
    if (!this.activeThread.cachedFrames.length) {
      DebuggerView.StackFrames.emptyText();
      return;
    }
    DebuggerView.StackFrames.empty();

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
    DebuggerView.StackFrames.emptyText();
    this.selectedFrame = null;

    
    DebuggerView.Properties.localScope.empty();
    DebuggerView.Properties.globalScope.empty();
  },

  



  updateEditorLocation: function SF_updateEditorLocation() {
    let frame = this.activeThread.cachedFrames[this.selectedFrame];
    if (!frame) {
      return;
    }

    let url = frame.where.url;
    let line = frame.where.line;
    let editor = DebuggerView.editor;

    
    if (DebuggerView.Scripts.isSelected(url) && line) {
      editor.setDebugLocation(line - 1);
    } else {
      editor.setDebugLocation(-1);
    }
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
    let editor = DebuggerView.editor;

    
    if (DebuggerView.Scripts.isSelected(url) && line) {
      editor.setCaretPosition(line - 1);
      editor.setDebugLocation(line - 1);
    }
    else if (DebuggerView.Scripts.contains(url)) {
      DebuggerView.Scripts.selectScript(url);
      editor.setCaretPosition(line - 1);
    }
    else {
      editor.setDebugLocation(-1);
    }

    
    let localScope = DebuggerView.Properties.localScope;
    localScope.empty();

    
    if (frame.this) {
      let thisVar = localScope.addVar("this");
      thisVar.setGrip({
        type: frame.this.type,
        class: frame.this.class
      });
      this._addExpander(thisVar, frame.this);
    }

    if (frame.arguments && frame.arguments.length > 0) {
      
      let argsVar = localScope.addVar("arguments");
      argsVar.setGrip({
        type: "object",
        class: "Arguments"
      });
      this._addExpander(argsVar, frame.arguments);

      
      let objClient = this.activeThread.pauseGrip(frame.callee);
      objClient.getSignature(function SF_getSignature(aResponse) {
        for (let i = 0, l = aResponse.parameters.length; i < l; i++) {
          let param = aResponse.parameters[i];
          let paramVar = localScope.addVar(param);
          let paramVal = frame.arguments[i];

          paramVar.setGrip(paramVal);
          this._addExpander(paramVar, paramVal);
        }

        
        DebuggerController.dispatchEvent("Debugger:FetchedParameters");

      }.bind(this));
    }
  },

  



  _addExpander: function SF__addExpander(aVar, aObject) {
    
    
    if (!aObject || typeof aObject !== "object" ||
        (aObject.type !== "object" && !Array.isArray(aObject))) {
      return;
    }

    
    aVar.forceShowArrow();
    aVar.onexpand = this._addVarProperties.bind(this, aVar, aObject);
  },

  



  _addVarProperties: function SF__addVarProperties(aVar, aObject) {
    
    if (aVar.fetched) {
      return;
    }

    
    if (Array.isArray(aObject)) {
      let properties = { length: { value: aObject.length } };
      for (let i = 0, l = aObject.length; i < l; i++) {
        properties[i] = { value: aObject[i] };
      }
      aVar.addProperties(properties);

      
      for (let i = 0, l = aObject.length; i < l; i++) {
        this._addExpander(aVar[i], aObject[i]);
      }

      aVar.fetched = true;
      return;
    }

    let objClient = this.activeThread.pauseGrip(aObject);
    objClient.getPrototypeAndProperties(function SF_onProtoAndProps(aResponse) {
      
      if (aResponse.prototype.type !== "null") {
        let properties = { "__proto__ ": { value: aResponse.prototype } };
        aVar.addProperties(properties);

        
        this._addExpander(aVar["__proto__ "], aResponse.prototype);
      }

      
      let properties = {};
      for each (let prop in Object.keys(aResponse.ownProperties).sort()) {
        properties[prop] = aResponse.ownProperties[prop];
      }
      aVar.addProperties(properties);

      
      for (let prop in aResponse.ownProperties) {
        this._addExpander(aVar[prop], aResponse.ownProperties[prop].value);
      }

      aVar.fetched = true;
    }.bind(this));
  },

  





  _addFrame: function SF__addFrame(aFrame) {
    let depth = aFrame.depth;
    let label = DebuggerController.SourceScripts._getScriptLabel(aFrame.where.url);

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
  }
};





function SourceScripts() {
  this._onNewScript = this._onNewScript.bind(this);
  this._onScriptsAdded = this._onScriptsAdded.bind(this);
  this._onScriptsCleared = this._onScriptsCleared.bind(this);
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
    this.activeThread.addListener("scriptsadded", this._onScriptsAdded);
    this.activeThread.addListener("scriptscleared", this._onScriptsCleared);

    this._clearLabelsCache();
    this._onScriptsCleared();

    
    
    this.activeThread.fillScripts();

    aCallback && aCallback();
  },

  


  disconnect: function TS_disconnect() {
    window.removeEventListener("Debugger:LoadSource", this._onLoadSource, false);

    if (!this.activeThread) {
      return;
    }
    this.debuggerClient.removeListener("newScript", this._onNewScript);
    this.activeThread.removeListener("scriptsadded", this._onScriptsAdded);
    this.activeThread.removeListener("scriptscleared", this._onScriptsCleared);
  },

  


  _onNewScript: function SS__onNewScript(aNotification, aPacket) {
    this._addScript({ url: aPacket.url, startLine: aPacket.startLine });
  },

  


  _onScriptsAdded: function SS__onScriptsAdded() {
    for each (let script in this.activeThread.cachedScripts) {
      this._addScript(script);
    }
  },

  


  _onScriptsCleared: function SS__onScriptsCleared() {
    DebuggerView.Scripts.empty();
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

    
    if (/\.jsm?$/.test(this._trimUrlQuery(aUrl))) {
      DebuggerView.editor.setMode(SourceEditor.MODES.JAVASCRIPT);
    } else {
      DebuggerView.editor.setMode(SourceEditor.MODES.HTML);
    }
  },

  






  _trimUrlQuery: function SS__trimUrlQuery(aUrl) {
    let q = aUrl.indexOf('?');
    if (q > -1) {
      return aUrl.slice(0, q);
    }
    return aUrl;
  },

  



















  _getScriptLabel: function SS__getScriptLabel(aUrl, aHref) {
    let url = this._trimUrlQuery(aUrl);

    if (this._labelsCache[url]) {
      return this._labelsCache[url];
    }

    let href = aHref || window.parent.content.location.href;
    let pathElements = url.split("/");
    let label = pathElements.pop() || (pathElements.pop() + "/");

    
    if (DebuggerView.Scripts.containsLabel(label)) {
      label = url.replace(href.substring(0, href.lastIndexOf("/") + 1), "");

      
      if (DebuggerView.Scripts.containsLabel(label)) {
        label = url;
      }
    }

    return this._labelsCache[url] = label;
  },

  



  _clearLabelsCache: function SS__clearLabelsCache() {
    this._labelsCache = {};
  },

  



  _addScript: function SS__addScript(aScript) {
    DebuggerView.Scripts.addScript(this._getScriptLabel(aScript.url), aScript);

    if (DebuggerView.editor.getCharCount() == 0) {
      this.showScript(aScript);
    }
  },

  









  showScript: function SS_showScript(aScript, aOptions) {
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

  









  _onShowScript: function SS__onShowScript(aScript, aOptions) {
    aOptions = aOptions || {};

    this._setEditorMode(aScript.url, aScript.contentType);

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
  function SS__onLoadSourceFinished(aSourceUrl, aSourceText, aContentType, aOptions) {
    let scripts = document.getElementById("scripts");
    let element = scripts.getElementsByAttribute("value", aSourceUrl)[0];
    let script = element.getUserData("sourceScript");

    script.loaded = true;
    script.text = aSourceText;
    script.contentType = aContentType;
    element.setUserData("sourceScript", script, null);

    this.showScript(script, aOptions);
  },

  







  _logError: function SS__logError(aUrl, aStatus) {
    Components.utils.reportError(L10N.getFormatStr("loadingError", [aUrl, aStatus]));
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

    let callback = function(aClient, aError) {
      if (aError) {
        this._skipEditorBreakpointChange = true;
        let result = this.editor.removeBreakpoint(aBreakpoint.line);
        this._skipEditorBreakpointChange = false;
      }
    }.bind(this);
    this.addBreakpoint({ url: url, line: line }, callback, true);
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

  

















  addBreakpoint:
  function BP_addBreakpoint(aLocation, aCallback, aNoEditorUpdate) {
    let breakpoint = this.getBreakpoint(aLocation.url, aLocation.line);
    if (breakpoint) {
      aCallback && aCallback(breakpoint);
      return;
    }

    this.activeThread.setBreakpoint(aLocation, function(aResponse, aBpClient) {
      if (!aResponse.error) {
        this.store[aBpClient.actor] = aBpClient;

        if (!aNoEditorUpdate) {
          let url = DebuggerView.Scripts.selected;
          if (url == aLocation.url) {
            this._skipEditorBreakpointChange = true;
            this.editor.addBreakpoint(aLocation.line - 1);
            this._skipEditorBreakpointChange = false;
          }
        }
      }

      aCallback && aCallback(aBpClient, aResponse.error);
    }.bind(this));
  },

  












  removeBreakpoint:
  function BP_removeBreakpoint(aBreakpoint, aCallback, aNoEditorUpdate) {
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
