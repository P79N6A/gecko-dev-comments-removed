




"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
                                  "resource://gre/modules/devtools/dbg-server.jsm");

let RemoteDebugger = {
  init() {
    USBRemoteDebugger.init();
    WiFiRemoteDebugger.init();
  },

  get isAnyEnabled() {
    return USBRemoteDebugger.isEnabled || WiFiRemoteDebugger.isEnabled;
  },

  





















  allowConnection(session) {
    if (this._promptingForAllow) {
      
      return DebuggerServer.AuthenticationResult.DENY;
    }

    if (!session.server.port) {
      this._promptingForAllow = this._promptForUSB(session);
    } else {
      this._promptingForAllow = this._promptForTCP(session);
    }
    this._promptingForAllow.then(() => this._promptingForAllow = null);

    return this._promptingForAllow;
  },

  _promptForUSB(session) {
    if (session.authentication !== 'PROMPT') {
      
      
      return DebuggerServer.AuthenticationResult.DENY;
    }

    return new Promise(resolve => {
      let title = Strings.browser.GetStringFromName("remoteIncomingPromptTitle");
      let msg = Strings.browser.GetStringFromName("remoteIncomingPromptUSB");
      let allow = Strings.browser.GetStringFromName("remoteIncomingPromptAllow");
      let deny = Strings.browser.GetStringFromName("remoteIncomingPromptDeny");

      
      let prompt = new Prompt({
        window: null,
        hint: "remotedebug",
        title: title,
        message: msg,
        buttons: [ allow, deny ],
        priority: 1
      });

      prompt.show(data => {
        let result = data.button;
        if (result === 0) {
          resolve(DebuggerServer.AuthenticationResult.ALLOW);
        } else {
          resolve(DebuggerServer.AuthenticationResult.DENY);
        }
      });
    });
  },

  _promptForTCP(session) {
    if (session.authentication !== 'OOB_CERT' || !session.client.cert) {
      
      
      return DebuggerServer.AuthenticationResult.DENY;
    }

    return new Promise(resolve => {
      let title = Strings.browser.GetStringFromName("remoteIncomingPromptTitle");
      let msg = Strings.browser.formatStringFromName("remoteIncomingPromptTCP", [
        session.client.host,
        session.client.port
      ], 2);
      let scan = Strings.browser.GetStringFromName("remoteIncomingPromptScan");
      let scanAndRemember = Strings.browser.GetStringFromName("remoteIncomingPromptScanAndRemember");
      let deny = Strings.browser.GetStringFromName("remoteIncomingPromptDeny");

      
      let prompt = new Prompt({
        window: null,
        hint: "remotedebug",
        title: title,
        message: msg,
        buttons: [ scan, scanAndRemember, deny ],
        priority: 1
      });

      prompt.show(data => {
        let result = data.button;
        if (result === 0) {
          resolve(DebuggerServer.AuthenticationResult.ALLOW);
        } else if (result === 1) {
          resolve(DebuggerServer.AuthenticationResult.ALLOW_PERSIST);
        } else {
          resolve(DebuggerServer.AuthenticationResult.DENY);
        }
      });
    });
  },

  












  receiveOOB() {
    if (this._receivingOOB) {
      return this._receivingOOB;
    }

    this._receivingOOB = Messaging.sendRequestForResult({
      type: "DevToolsAuth:Scan"
    }).then(data => {
      return JSON.parse(data);
    });

    this._receivingOOB.then(() => this._receivingOOB = null);

    return this._receivingOOB;
  },

  initServer: function() {
    if (DebuggerServer.initialized) {
      return;
    }

    DebuggerServer.init();

    
    DebuggerServer.addBrowserActors();
    DebuggerServer.registerModule("resource://gre/modules/dbg-browser-actors.js");

    
    DebuggerServer.allowChromeProcess = true;
  }
};

RemoteDebugger.allowConnection =
  RemoteDebugger.allowConnection.bind(RemoteDebugger);
RemoteDebugger.receiveOOB =
  RemoteDebugger.receiveOOB.bind(RemoteDebugger);

let USBRemoteDebugger = {

  init() {
    Services.prefs.addObserver("devtools.", this, false);

    if (this.isEnabled) {
      this.start();
    }
  },

  observe(subject, topic, data) {
    if (topic != "nsPref:changed") {
      return;
    }

    switch (data) {
      case "devtools.remote.usb.enabled":
        Services.prefs.setBoolPref("devtools.debugger.remote-enabled",
                                   RemoteDebugger.isAnyEnabled);
        if (this.isEnabled) {
          this.start();
        } else {
          this.stop();
        }
        break;

      case "devtools.debugger.remote-port":
      case "devtools.debugger.unix-domain-socket":
        if (this.isEnabled) {
          this.stop();
          this.start();
        }
        break;
    }
  },

  get isEnabled() {
    return Services.prefs.getBoolPref("devtools.remote.usb.enabled");
  },

  start: function() {
    if (this._listener) {
      return;
    }

    RemoteDebugger.initServer();

    let portOrPath =
      Services.prefs.getCharPref("devtools.debugger.unix-domain-socket") ||
      Services.prefs.getIntPref("devtools.debugger.remote-port");

    try {
      dump("Starting USB debugger on " + portOrPath);
      let AuthenticatorType = DebuggerServer.Authenticators.get("PROMPT");
      let authenticator = new AuthenticatorType.Server();
      authenticator.allowConnection = RemoteDebugger.allowConnection;
      this._listener = DebuggerServer.createListener();
      this._listener.portOrPath = portOrPath;
      this._listener.authenticator = authenticator;
      this._listener.open();
    } catch (e) {
      dump("Unable to start USB debugger server: " + e);
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
      dump("Unable to stop USB debugger server: " + e);
    }
  }

};

let WiFiRemoteDebugger = {

  init() {
    Services.prefs.addObserver("devtools.", this, false);

    if (this.isEnabled) {
      this.start();
    }
  },

  observe(subject, topic, data) {
    if (topic != "nsPref:changed") {
      return;
    }

    switch (data) {
      case "devtools.remote.wifi.enabled":
        Services.prefs.setBoolPref("devtools.debugger.remote-enabled",
                                   RemoteDebugger.isAnyEnabled);
        
        
        
        Services.prefs.setBoolPref("devtools.debugger.force-local",
                                   !this.isEnabled);
        if (this.isEnabled) {
          this.start();
        } else {
          this.stop();
        }
        break;
    }
  },

  get isEnabled() {
    return Services.prefs.getBoolPref("devtools.remote.wifi.enabled");
  },

  start: function() {
    if (this._listener) {
      return;
    }

    RemoteDebugger.initServer();

    try {
      dump("Starting WiFi debugger");
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
      dump("Started WiFi debugger on " + port);
    } catch (e) {
      dump("Unable to start WiFi debugger server: " + e);
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
      dump("Unable to stop WiFi debugger server: " + e);
    }
  }

};
