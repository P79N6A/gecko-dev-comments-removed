








































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
Cu.import("resource:///modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/source-editor.jsm");

let EXPORTED_SYMBOLS = ["DebuggerUI"];




function DebuggerPane(aTab) {
  this._tab = aTab;
  this._close = this.close.bind(this);
  this._debugTab = this.debugTab.bind(this);
  this.breakpoints = {};
}

DebuggerPane.prototype = {
  














  _skipEditorBreakpointChange: false,

  







  breakpoints: null,

  


  create: function DP_create(gBrowser) {
    this._tab._scriptDebugger = this;

    this._nbox = gBrowser.getNotificationBox(this._tab.linkedBrowser);
    this._splitter = gBrowser.parentNode.ownerDocument.createElement("splitter");
    this._splitter.setAttribute("class", "hud-splitter");
    this.frame = gBrowser.parentNode.ownerDocument.createElement("iframe");
    this.frame.height = DebuggerUIPreferences.height;

    this._nbox.appendChild(this._splitter);
    this._nbox.appendChild(this.frame);

    let self = this;

    this.frame.addEventListener("DOMContentLoaded", function initPane(aEvent) {
      if (aEvent.target != self.frame.contentDocument) {
        return;
      }
      self.frame.removeEventListener("DOMContentLoaded", initPane, true);
      
      self.frame.contentWindow.editor = self.editor = new SourceEditor();
      self.frame.contentWindow.updateEditorBreakpoints =
        self._updateEditorBreakpoints.bind(self);

      let config = {
        mode: SourceEditor.MODES.JAVASCRIPT,
        showLineNumbers: true,
        readOnly: true,
        showAnnotationRuler: true,
        showOverviewRuler: true,
      };

      let editorPlaceholder = self.frame.contentDocument.getElementById("editor");
      self.editor.init(editorPlaceholder, config, self._onEditorLoad.bind(self));
    }, true);
    this.frame.addEventListener("DebuggerClose", this._close, true);

    this.frame.setAttribute("src", "chrome://browser/content/debugger.xul");
  },

  



  _onEditorLoad: function DP__onEditorLoad() {
    this.editor.addEventListener(SourceEditor.EVENTS.BREAKPOINT_CHANGE,
                                 this._onEditorBreakpointChange.bind(this));
    
    this.connect();
  },

  








  _onEditorBreakpointChange: function DP__onEditorBreakpointChange(aEvent) {
    if (this._skipEditorBreakpointChange) {
      return;
    }

    aEvent.added.forEach(this._onEditorBreakpointAdd, this);
    aEvent.removed.forEach(this._onEditorBreakpointRemove, this);
  },

  






  _selectedScript: function DP__selectedScript() {
    return this.debuggerWindow ?
           this.debuggerWindow.DebuggerView.Scripts.selected : null;
  },

  






  _onEditorBreakpointAdd: function DP__onEditorBreakpointAdd(aBreakpoint) {
    let location = {
      url: this._selectedScript(),
      line: aBreakpoint.line + 1,
    };

    if (location.url) {
      let callback = function (aClient, aError) {
        if (aError) {
          this._skipEditorBreakpointChange = true;
          let result = this.editor.removeBreakpoint(aBreakpoint.line);
          this._skipEditorBreakpointChange = false;
        }
      }.bind(this);
      this.addBreakpoint(location, callback, true);
    }
  },

  






  _onEditorBreakpointRemove: function DP__onEditorBreakpointRemove(aBreakpoint) {
    let url = this._selectedScript();
    let line = aBreakpoint.line + 1;
    if (!url) {
      return;
    }

    let breakpoint = this.getBreakpoint(url, line);
    if (breakpoint) {
      this.removeBreakpoint(breakpoint, null, true);
    }
  },

  






  _updateEditorBreakpoints: function DP__updateEditorBreakpoints()
  {
    let url = this._selectedScript();
    if (!url) {
      return;
    }

    this._skipEditorBreakpointChange = true;
    for each (let breakpoint in this.breakpoints) {
      if (breakpoint.location.url == url) {
        this.editor.addBreakpoint(breakpoint.location.line - 1);
      }
    }
    this._skipEditorBreakpointChange = false;
  },

  

















  addBreakpoint:
  function DP_addBreakpoint(aLocation, aCallback, aNoEditorUpdate) {
    let breakpoint = this.getBreakpoint(aLocation.url, aLocation.line);
    if (breakpoint) {
      aCallback && aCallback(breakpoint);
      return;
    }

    this.activeThread.setBreakpoint(aLocation, function(aResponse, aBpClient) {
      if (!aResponse.error) {
        this.breakpoints[aBpClient.actor] = aBpClient;

        if (!aNoEditorUpdate) {
          let url = this._selectedScript();
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
  function DP_removeBreakpoint(aBreakpoint, aCallback, aNoEditorUpdate) {
    if (!(aBreakpoint.actor in this.breakpoints)) {
      aCallback && aCallback(aBreakpoint.location);
      return;
    }

    aBreakpoint.remove(function() {
      delete this.breakpoints[aBreakpoint.actor];

      if (!aNoEditorUpdate) {
        let url = this._selectedScript();
        if (url == aBreakpoint.location.url) {
          this._skipEditorBreakpointChange = true;
          this.editor.removeBreakpoint(aBreakpoint.location.line - 1);
          this._skipEditorBreakpointChange = false;
        }
      }

      aCallback && aCallback(aBreakpoint.location);
    }.bind(this));
  },

  









  getBreakpoint: function DP_getBreakpoint(aUrl, aLine) {
    for each (let breakpoint in this.breakpoints) {
      if (breakpoint.location.url == aUrl && breakpoint.location.line == aLine) {
        return breakpoint;
      }
    }
    return null;
  },

  


  close: function DP_close() {
    for each (let breakpoint in this.breakpoints) {
      this.removeBreakpoint(breakpoint);
    }

    if (this._tab) {
      this._tab._scriptDebugger = null;
      this._tab = null;
    }
    if (this.frame) {
      DebuggerUIPreferences.height = this.frame.height;

      this.frame.removeEventListener("unload", this._close, true);
      this.frame.removeEventListener("DebuggerClose", this._close, true);
      if (this.frame.parentNode) {
        this.frame.parentNode.removeChild(this.frame);
      }
      this.frame = null;
    }
    if (this._nbox) {
      this._nbox.removeChild(this._splitter);
      this._nbox = null;
    }

    this._splitter = null;

    if (this._client) {
      this._client.removeListener("newScript", this.onNewScript);
      this._client.removeListener("tabDetached", this._close);
      this._client.removeListener("tabNavigated", this._debugTab);
      this._client.close();
      this._client = null;
    }
  },

  



  connect: function DP_connect() {
    this.frame.addEventListener("unload", this._close, true);

    let transport = DebuggerServer.connectPipe();
    this._client = new DebuggerClient(transport);
    
    
    this.onNewScript = this.debuggerWindow.SourceScripts.onNewScript;
    let self = this;
    this._client.addListener("tabNavigated", this._debugTab);
    this._client.addListener("tabDetached", this._close);
    this._client.addListener("newScript", this.onNewScript);
    this._client.connect(function(aType, aTraits) {
      self._client.listTabs(function(aResponse) {
        let tab = aResponse.tabs[aResponse.selected];
        self.debuggerWindow.startDebuggingTab(self._client, tab);
        if (self.onConnected) {
          self.onConnected(self);
        }
      });
    });
  },

  



  debugTab: function DP_debugTab(aNotification, aPacket) {
    let self = this;
    this._client.activeThread.detach(function() {
      self._client.activeTab.detach(function() {
        self._client.listTabs(function(aResponse) {
          let tab = aResponse.tabs[aResponse.selected];
          self.debuggerWindow.startDebuggingTab(self._client, tab);
          if (self.onConnected) {
            self.onConnected(self);
          }
        });
      });
    });
  },

  get debuggerWindow() {
    return this.frame ? this.frame.contentWindow : null;
  },

  get debuggerClient() {
    return this._client;
  },

  get activeThread() {
    try {
      return this.debuggerWindow.ThreadState.activeThread;
    } catch(ex) {
      return undefined;
    }
  }
};

function DebuggerUI(aWindow) {
  this.aWindow = aWindow;

  aWindow.addEventListener("Debugger:LoadSource", this._onLoadSource.bind(this));
}

DebuggerUI.prototype = {
  


  toggleDebugger: function DebuggerUI_toggleDebugger() {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
    }

    let gBrowser = this.aWindow.gBrowser;
    let tab = gBrowser.selectedTab;

    if (tab._scriptDebugger) {
      
      tab._scriptDebugger.close();
      return tab._scriptDebugger;
    }

    let pane = new DebuggerPane(tab);
    pane.create(gBrowser);
    return pane;
  },

  getDebugger: function DebuggerUI_getDebugger(aTab) {
    return aTab._scriptDebugger;
  },

  get preferences() {
    return DebuggerUIPreferences;
  },

  






  _onLoadSource: function DebuggerUI__onLoadSource(aEvent) {
    let gBrowser = this.aWindow.gBrowser;

    let url = aEvent.detail;
    let scheme = Services.io.extractScheme(url);
    switch (scheme) {
      case "file":
      case "chrome":
      case "resource":
        try {
          NetUtil.asyncFetch(url, function onFetch(aStream, aStatus) {
            if (!Components.isSuccessCode(aStatus)) {
              return this.logError(url, aStatus);
            }
            let source = NetUtil.readInputStreamToString(aStream, aStream.available());
            aStream.close();
            this._onSourceLoaded(url, source);
          }.bind(this));
        } catch (ex) {
          return this.logError(url, ex.name);
        }
        break;

      default:
        let channel = Services.io.newChannel(url, null, null);
        let chunks = [];
        let streamListener = { 
          onStartRequest: function (aRequest, aContext, aStatusCode) {
            if (!Components.isSuccessCode(aStatusCode)) {
              return this.logError(url, aStatusCode);
            }
          }.bind(this),
          onDataAvailable: function (aRequest, aContext, aStream, aOffset, aCount) {
            chunks.push(NetUtil.readInputStreamToString(aStream, aCount));
          },
          onStopRequest: function (aRequest, aContext, aStatusCode) {
            if (!Components.isSuccessCode(aStatusCode)) {
              return this.logError(url, aStatusCode);
            }

            this._onSourceLoaded(url, chunks.join(""), channel.contentType);
          }.bind(this)
        };

        channel.loadFlags = channel.LOAD_FROM_CACHE;
        channel.asyncOpen(streamListener, null);
        break;
    }
  },

  







  logError: function DebuggerUI_logError(aUrl, aStatus) {
    let view = this.getDebugger(gBrowser.selectedTab).DebuggerView;
    Components.utils.reportError(view.getFormatStr("loadingError", [ aUrl, aStatus ]));
  },

  









  _onSourceLoaded: function DebuggerUI__onSourceLoaded(aSourceUrl,
                                                       aSourceText,
                                                       aContentType) {
    let dbg = this.getDebugger(this.aWindow.gBrowser.selectedTab);
    dbg.debuggerWindow.SourceScripts.setEditorMode(aSourceUrl, aContentType);
    dbg.editor.setText(aSourceText);
    dbg.editor.resetUndo();
    let doc = dbg.frame.contentDocument;
    let scripts = doc.getElementById("scripts");
    let elt = scripts.getElementsByAttribute("value", aSourceUrl)[0];
    let script = elt.getUserData("sourceScript");
    script.loaded = true;
    script.text = aSourceText;
    script.contentType = aContentType;
    elt.setUserData("sourceScript", script, null);
    dbg._updateEditorBreakpoints();
    dbg.debuggerWindow.StackFrames.updateEditor();
  }
};




let DebuggerUIPreferences = {

  _height: -1,

  





  get height() {
    if (this._height < 0) {
      this._height = Services.prefs.getIntPref("devtools.debugger.ui.height");
    }
    return this._height;
  },

  





  set height(value) {
    Services.prefs.setIntPref("devtools.debugger.ui.height", value);
    this._height = value;
  }
};
