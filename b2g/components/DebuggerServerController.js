




const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this,
    "Settings",
    "@mozilla.org/settingsService;1", "nsISettingsService");

XPCOMUtils.defineLazyModuleGetter(this,
     "SystemAppProxy",
     "resource://gre/modules/SystemAppProxy.jsm");

function DebuggerServerController() {
}

DebuggerServerController.prototype = {
  classID: Components.ID("{9390f6ac-7914-46c6-b9d0-ccc7db550d8c}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDebuggerServerController, Ci.nsIObserver]),

  init: function init(debuggerServer) {
    this.debugger = debuggerServer;
    Services.obs.addObserver(this, "mozsettings-changed", false);
    Services.obs.addObserver(this, "debugger-server-started", false);
    Services.obs.addObserver(this, "debugger-server-stopped", false);
    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  uninit: function uninit() {
    this.debugger = null;
    Services.obs.removeObserver(this, "mozsettings-changed");
    Services.obs.removeObserver(this, "debugger-server-started");
    Services.obs.removeObserver(this, "debugger-server-stopped");
    Services.obs.removeObserver(this, "xpcom-shutdown");
  },

  

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "xpcom-shutdown":
        this.uninit();
        break;
      case "debugger-server-started":
        this._onDebuggerStarted(data);
        break;
      case "debugger-server-stopped":
        this._onDebuggerStopped();
        break;
      case "mozsettings-changed":
        let {key, value} = JSON.parse(data);
        switch(key) {
          case "debugger.remote-mode":
            if (["disabled", "adb-only", "adb-devtools"].indexOf(value) == -1) {
              dump("Illegal value for debugger.remote-mode: " + value + "\n");
              return;
            }

            Services.prefs.setBoolPref("devtools.debugger.remote-enabled", value == "adb-devtools");
            Services.prefs.savePrefFile(null);

            if (value != "adb-devtools") {
              
              
              
              
              
              this.stop();
            }
        }
    }

  },

  

  start: function(portOrPath) {
    if (!portOrPath) {
      throw new Error("No TCP port or unix socket path specified.");
    }

    if (!this.debugger.initialized) {
      
      this.debugger.init(Prompt.prompt.bind(Prompt));

      
      

      
      
      let restrictPrivileges = Services.prefs.getBoolPref("devtools.debugger.forbid-certified-apps");
      this.debugger.addBrowserActors("navigator:browser", restrictPrivileges);

      








      let debuggerServer = this.debugger;
      debuggerServer.createRootActor = function createRootActor(connection)
      {
        let { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
        let parameters = {
          
          
          
          
          tabList: {
            getList: function() {
              return promise.resolve([]);
            }
          },
          
          
          globalActorFactories: restrictPrivileges ? {
            webappsActor: debuggerServer.globalActorFactories.webappsActor,
            deviceActor: debuggerServer.globalActorFactories.deviceActor,
          } : debuggerServer.globalActorFactories
        };
        let root = new debuggerServer.RootActor(connection, parameters);
        root.applicationType = "operating-system";
        return root;
      };

    }

    try {
      this.debugger.openListener(portOrPath);
    } catch (e) {
      dump("Unable to start debugger server (" + portOrPath + "): " + e + "\n");
    }

  },

  stop: function() {
    this.debugger.destroy();
  },

  _onDebuggerStarted: function(portOrPath) {
    dump("Devtools debugger server started: " + portOrPath + "\n");
    Settings.createLock().set("debugger.remote-mode", "adb-devtools", null);
  },


  _onDebuggerStopped: function() {
    dump("Devtools debugger server stopped\n");
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DebuggerServerController]);



let Prompt = {
  _promptDone: false,
  _promptAnswer: false,
  _listenerAttached: false,

  prompt: function () {
    if (!this._listenerAttached) {
      SystemAppProxy.addEventListener("mozContentEvent", this, false, true);
      this._listenerAttached = true;
    }

    this._promptDone = false;

    SystemAppProxy._sendCustomEvent("mozChromeEvent", {
      "type": "remote-debugger-prompt"
    });


    while(!this._promptDone) {
      Services.tm.currentThread.processNextEvent(true);
    }

    return this._promptAnswer;
  },

  

  handleEvent: function (event) {
    if (event.detail.type == "remote-debugger-prompt") {
      this._promptAnswer = event.detail.value;
      this._promptDone = true;
    }
  }
}
