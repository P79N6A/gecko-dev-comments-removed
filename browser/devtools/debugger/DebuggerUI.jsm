







































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
}

DebuggerPane.prototype = {
  


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

      let config = {
        mode: SourceEditor.MODES.JAVASCRIPT,
        showLineNumbers: true,
        readOnly: true
      };

      let editorPlaceholder = self.frame.contentDocument.getElementById("editor");
      self.editor.init(editorPlaceholder, config, self._onEditorLoad.bind(self));
    }, true);
    this.frame.addEventListener("DebuggerClose", this._close, true);

    this.frame.setAttribute("src", "chrome://browser/content/debugger.xul");
  },

  



  _onEditorLoad: function DP__onEditorLoad() {
    
    this.connect();
  },

  


  close: function DP_close() {
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
    return this.frame.contentWindow;
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

            this._onSourceLoaded(url, chunks.join(""));
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

  







  _onSourceLoaded: function DebuggerUI__onSourceLoaded(aSourceUrl, aSourceText) {
    let dbg = this.getDebugger(this.aWindow.gBrowser.selectedTab);
    if (aSourceUrl.slice(-3) == ".js") {
      dbg.editor.setMode(SourceEditor.MODES.JAVASCRIPT);
    } else {
      dbg.editor.setMode(SourceEditor.MODES.HTML);
    }
    dbg.editor.setText(aSourceText);
    let doc = dbg.frame.contentDocument;
    let scripts = doc.getElementById("scripts");
    let elt = scripts.getElementsByAttribute("value", aSourceUrl)[0];
    let script = elt.getUserData("sourceScript");
    script.loaded = true;
    script.text = aSourceText;
    elt.setUserData("sourceScript", script, null);
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
