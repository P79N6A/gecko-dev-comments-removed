





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

XPCOMUtils.defineLazyGetter(this, "B2GTabList", function() {
  const { B2GTabList } =
    devtools.require("resource://gre/modules/DebuggerActors.js");
  return B2GTabList;
});

let RemoteDebugger = {
  _listening: false,

  





















  allowConnection(session) {
    if (this._promptingForAllow) {
      
      return DebuggerServer.AuthenticationResult.DENY;
    }
    this._listen();

    this._promptingForAllow = new Promise(resolve => {
      this._handleAllowResult = detail => {
        this._handleAllowResult = null;
        this._promptingForAllow = null;
        if (detail.value) {
          resolve(DebuggerServer.AuthenticationResult.ALLOW);
        } else {
          resolve(DebuggerServer.AuthenticationResult.DENY);
        }
      };

      shell.sendChromeEvent({
        type: "remote-debugger-prompt",
        session
      });
    });

    return this._promptingForAllow;
  },

  












  receiveOOB() {
    if (this._receivingOOB) {
      return this._receivingOOB;
    }
    this._listen();

    const QR = devtools.require("devtools/toolkit/qrcode/index");
    this._receivingOOB = new Promise((resolve, reject) => {
      this._handleAuthEvent = detail => {
        debug(detail.action);
        if (detail.action === "abort") {
          this._handleAuthEvent = null;
          this._receivingOOB = null;
          reject();
          return;
        }

        if (detail.action !== "capture") {
          return;
        }

        let url = detail.url;
        QR.decodeFromURI(url).then(data => {
          debug("Got auth data: " + data);
          let oob = JSON.parse(data);

          shell.sendChromeEvent({
            type: "devtools-auth",
            action: "stop"
          });

          this._handleAuthEvent = null;
          this._receivingOOB = null;
          resolve(oob);
        }).catch(() => {
          debug("No auth data, requesting new capture");
          shell.sendChromeEvent({
            type: "devtools-auth",
            action: "capture"
          });
        });
      };

      
      shell.sendChromeEvent({
        type: "devtools-auth",
        action: "start"
      });
    });

    return this._receivingOOB;
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
    if (detail.type === "remote-debugger-prompt" && this._handleAllowResult) {
      this._handleAllowResult(detail);
    }
    if (detail.type === "devtools-auth" && this._handleAuthEvent) {
      this._handleAuthEvent(detail);
    }
  },

  initServer: function() {
    if (DebuggerServer.initialized) {
      return;
    }

    
    DebuggerServer.init();

    
    

    
    
    let restrictPrivileges = Services.prefs.getBoolPref("devtools.debugger.forbid-certified-apps");
    DebuggerServer.addBrowserActors("navigator:browser", restrictPrivileges);

    








    DebuggerServer.createRootActor = function createRootActor(connection)
    {
      let parameters = {
        tabList: new B2GTabList(connection),
        
        
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

RemoteDebugger.allowConnection =
  RemoteDebugger.allowConnection.bind(RemoteDebugger);
RemoteDebugger.receiveOOB =
  RemoteDebugger.receiveOOB.bind(RemoteDebugger);

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
      let AuthenticatorType = DebuggerServer.Authenticators.get("PROMPT");
      let authenticator = new AuthenticatorType.Server();
      authenticator.allowConnection = RemoteDebugger.allowConnection;
      this._listener = DebuggerServer.createListener();
      this._listener.portOrPath = portOrPath;
      this._listener.authenticator = authenticator;
      this._listener.open();
      
      
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
      let AuthenticatorType = DebuggerServer.Authenticators.get("OOB_CERT");
      let authenticator = new AuthenticatorType.Server();
      authenticator.allowConnection = RemoteDebugger.allowConnection;
      authenticator.receiveOOB = RemoteDebugger.receiveOOB;
      this._listener = DebuggerServer.createListener();
      this._listener.portOrPath = -1 ;
      this._listener.authenticator = authenticator;
      this._listener.discoverable = true;
      this._listener.encryption = true;
      this._listener.open();
      let port = this._listener.port;
      debug("Started WiFi debugger on " + port);
    } catch (e) {
      debug("Unable to start WiFi debugger server: " + e);
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
