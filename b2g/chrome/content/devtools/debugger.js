





"use strict";

XPCOMUtils.defineLazyGetter(this, "DebuggerServer", function() {
  Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
  return DebuggerServer;
});

XPCOMUtils.defineLazyGetter(this, "devtools", function() {
  const { devtools } =
    Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
  return devtools;
});

XPCOMUtils.defineLazyGetter(this, "discovery", function() {
  return devtools.require("devtools/toolkit/discovery/discovery");
});

let RemoteDebugger = {
  _promptDone: false,
  _promptAnswer: false,
  _listening: false,

  prompt: function() {
    this._listen();

    this._promptDone = false;

    shell.sendChromeEvent({
      "type": "remote-debugger-prompt"
    });

    while(!this._promptDone) {
      Services.tm.currentThread.processNextEvent(true);
    }

    return this._promptAnswer;
  },

  _listen: function() {
    if (this._listening) {
      return;
    }

    this.handleEvent = this.handleEvent.bind(this);
    let content = shell.contentBrowser.contentWindow;
    content.addEventListener("mozContentEvent", this, false, true);
    this._listening = true;
  },

  handleEvent: function(event) {
    let detail = event.detail;
    if (detail.type !== "remote-debugger-prompt") {
      return;
    }
    this._promptAnswer = detail.value;
    this._promptDone = true;
  },

  initServer: function() {
    if (DebuggerServer.initialized) {
      return;
    }

    
    DebuggerServer.init(this.prompt.bind(this));

    
    

    
    
    let restrictPrivileges = Services.prefs.getBoolPref("devtools.debugger.forbid-certified-apps");
    DebuggerServer.addBrowserActors("navigator:browser", restrictPrivileges);

    








    DebuggerServer.createRootActor = function createRootActor(connection)
    {
      let { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
      let parameters = {
        
        
        
        
        tabList: {
          getList: function() {
            return promise.resolve([]);
          }
        },
        
        
        globalActorFactories: restrictPrivileges ? {
          webappsActor: DebuggerServer.globalActorFactories.webappsActor,
          deviceActor: DebuggerServer.globalActorFactories.deviceActor,
        } : DebuggerServer.globalActorFactories
      };
      let { RootActor } = devtools.require("devtools/server/actors/root");
      let root = new RootActor(connection, parameters);
      root.applicationType = "operating-system";
      return root;
    };

#ifdef MOZ_WIDGET_GONK
    DebuggerServer.on("connectionchange", function() {
      AdbController.updateState();
    });
#endif
  }
};

let USBRemoteDebugger = {

  get isDebugging() {
    if (!this._listener) {
      return false;
    }

    return DebuggerServer._connections &&
           Object.keys(DebuggerServer._connections).length > 0;
  },

  start: function() {
    if (this._listener) {
      return;
    }

    RemoteDebugger.initServer();

    let portOrPath =
      Services.prefs.getCharPref("devtools.debugger.unix-domain-socket") ||
      "/data/local/debugger-socket";

    try {
      debug("Starting USB debugger on " + portOrPath);
      this._listener = DebuggerServer.openListener(portOrPath);
      
      
      Services.obs.notifyObservers(null, "debugger-server-started", null);
    } catch (e) {
      debug("Unable to start USB debugger server: " + e);
    }
  },

  stop: function() {
    if (!this._listener) {
      return;
    }

    try {
      this._listener.close();
      this._listener = null;
    } catch (e) {
      debug("Unable to stop USB debugger server: " + e);
    }
  }

};

let WiFiRemoteDebugger = {

  start: function() {
    if (this._listener) {
      return;
    }

    RemoteDebugger.initServer();

    try {
      debug("Starting WiFi debugger");
      this._listener = DebuggerServer.openListener(-1);
      let port = this._listener.port;
      debug("Started WiFi debugger on " + port);
      discovery.addService("devtools", { port: port });
    } catch (e) {
      debug("Unable to start WiFi debugger server: " + e);
    }
  },

  stop: function() {
    if (!this._listener) {
      return;
    }

    try {
      discovery.removeService("devtools");
      this._listener.close();
      this._listener = null;
    } catch (e) {
      debug("Unable to stop WiFi debugger server: " + e);
    }
  }

};

(function() {
  
  
  
  let devtoolsUSB = false;
  let devtoolsWiFi = false;

  
  
  SettingsListener.observe("devtools.debugger.remote-enabled", false,
                           function(value) {
    devtoolsUSB = value;
    Services.prefs.setBoolPref("devtools.debugger.remote-enabled",
                               devtoolsUSB || devtoolsWiFi);
    
    Services.prefs.savePrefFile(null);
    try {
      value ? USBRemoteDebugger.start() : USBRemoteDebugger.stop();
    } catch(e) {
      dump("Error while initializing USB devtools: " +
           e + "\n" + e.stack + "\n");
    }
  });

  SettingsListener.observe("debugger.remote-mode", "disabled", function(value) {
    if (["disabled", "adb-only", "adb-devtools"].indexOf(value) == -1) {
      dump("Illegal value for debugger.remote-mode: " + value + "\n");
      return;
    }

    devtoolsUSB = value == "adb-devtools";
    Services.prefs.setBoolPref("devtools.debugger.remote-enabled",
                               devtoolsUSB || devtoolsWiFi);
    
    Services.prefs.savePrefFile(null);

    try {
      (value == "adb-devtools") ? USBRemoteDebugger.start()
                                : USBRemoteDebugger.stop();
    } catch(e) {
      dump("Error while initializing USB devtools: " +
           e + "\n" + e.stack + "\n");
    }

#ifdef MOZ_WIDGET_GONK
    AdbController.setRemoteDebuggerState(value != "disabled");
#endif
  });

  SettingsListener.observe("devtools.remote.wifi.enabled", false,
                           function(value) {
    devtoolsWiFi = value;
    Services.prefs.setBoolPref("devtools.debugger.remote-enabled",
                               devtoolsUSB || devtoolsWiFi);
    
    
    Services.prefs.setBoolPref("devtools.debugger.force-local", !value);
    
    Services.prefs.savePrefFile(null);

    try {
      value ? WiFiRemoteDebugger.start() : WiFiRemoteDebugger.stop();
    } catch(e) {
      dump("Error while initializing WiFi devtools: " +
           e + "\n" + e.stack + "\n");
    }
  });
})();
