



const {Cu, Ci} = require("chrome");
const {Devices} = Cu.import("resource://gre/modules/devtools/Devices.jsm");
const {Services} = Cu.import("resource://gre/modules/Services.jsm");
const {Connection} = require("devtools/client/connection-manager");
const {DebuggerServer} = require("resource://gre/modules/devtools/dbg-server.jsm");
const {Simulators} = require("devtools/webide/simulators");
const discovery = require("devtools/toolkit/discovery/discovery");
const EventEmitter = require("devtools/toolkit/event-emitter");
const promise = require("promise");
loader.lazyRequireGetter(this, "AuthenticationResult",
  "devtools/toolkit/security/auth", true);
loader.lazyRequireGetter(this, "DevToolsUtils",
  "devtools/toolkit/DevToolsUtils");

const Strings = Services.strings.createBundle("chrome://browser/locale/devtools/webide.properties");



































































let RuntimeScanners = {

  _enabledCount: 0,
  _scanners: new Set(),

  get enabled() {
    return !!this._enabledCount;
  },

  add(scanner) {
    if (this.enabled) {
      
      this._enableScanner(scanner);
    }
    this._scanners.add(scanner);
    this._emitUpdated();
  },

  remove(scanner) {
    this._scanners.delete(scanner);
    if (this.enabled) {
      
      this._disableScanner(scanner);
    }
    this._emitUpdated();
  },

  has(scanner) {
    return this._scanners.has(scanner);
  },

  scan() {
    if (!this.enabled) {
      return promise.resolve();
    }

    if (this._scanPromise) {
      return this._scanPromise;
    }

    let promises = [];

    for (let scanner of this._scanners) {
      promises.push(scanner.scan());
    }

    this._scanPromise = promise.all(promises);

    
    this._scanPromise.then(() => {
      this._scanPromise = null;
    }, () => {
      this._scanPromise = null;
    });

    return this._scanPromise;
  },

  listRuntimes: function*() {
    for (let scanner of this._scanners) {
      for (let runtime of scanner.listRuntimes()) {
        yield runtime;
      }
    }
  },

  _emitUpdated() {
    this.emit("runtime-list-updated");
  },

  enable() {
    if (this._enabledCount++ !== 0) {
      
      return;
    }
    this._emitUpdated = this._emitUpdated.bind(this);
    for (let scanner of this._scanners) {
      this._enableScanner(scanner);
    }
  },

  _enableScanner(scanner) {
    scanner.enable();
    scanner.on("runtime-list-updated", this._emitUpdated);
  },

  disable() {
    if (--this._enabledCount !== 0) {
      
      return;
    }
    for (let scanner of this._scanners) {
      this._disableScanner(scanner);
    }
  },

  _disableScanner(scanner) {
    scanner.off("runtime-list-updated", this._emitUpdated);
    scanner.disable();
  },

};

EventEmitter.decorate(RuntimeScanners);

exports.RuntimeScanners = RuntimeScanners;



let SimulatorScanner = {

  _runtimes: [],

  enable() {
    this._updateRuntimes = this._updateRuntimes.bind(this);
    Simulators.on("updated", this._updateRuntimes);
    this._updateRuntimes();
  },

  disable() {
    Simulators.off("updated", this._updateRuntimes);
  },

  _emitUpdated() {
    this.emit("runtime-list-updated");
  },

  _updateRuntimes() {
    Simulators.getAll().then(simulators => {
      this._runtimes = [];
      for (let simulator of simulators) {
        this._runtimes.push(new SimulatorRuntime(simulator));
      }
      this._emitUpdated();
    });
  },

  scan() {
    return promise.resolve();
  },

  listRuntimes: function() {
    return this._runtimes;
  }

};

EventEmitter.decorate(SimulatorScanner);
RuntimeScanners.add(SimulatorScanner);









let DeprecatedAdbScanner = {

  _runtimes: [],

  enable() {
    this._updateRuntimes = this._updateRuntimes.bind(this);
    Devices.on("register", this._updateRuntimes);
    Devices.on("unregister", this._updateRuntimes);
    Devices.on("addon-status-updated", this._updateRuntimes);
    this._updateRuntimes();
  },

  disable() {
    Devices.off("register", this._updateRuntimes);
    Devices.off("unregister", this._updateRuntimes);
    Devices.off("addon-status-updated", this._updateRuntimes);
  },

  _emitUpdated() {
    this.emit("runtime-list-updated");
  },

  _updateRuntimes() {
    this._runtimes = [];
    for (let id of Devices.available()) {
      let runtime = new DeprecatedUSBRuntime(id);
      this._runtimes.push(runtime);
      runtime.updateNameFromADB().then(() => {
        this._emitUpdated();
      }, () => {});
    }
    this._emitUpdated();
  },

  scan() {
    return promise.resolve();
  },

  listRuntimes: function() {
    return this._runtimes;
  }

};

EventEmitter.decorate(DeprecatedAdbScanner);
RuntimeScanners.add(DeprecatedAdbScanner);


exports.DeprecatedAdbScanner = DeprecatedAdbScanner;

let WiFiScanner = {

  _runtimes: [],

  init() {
    this.updateRegistration();
    Services.prefs.addObserver(this.ALLOWED_PREF, this, false);
  },

  enable() {
    this._updateRuntimes = this._updateRuntimes.bind(this);
    discovery.on("devtools-device-added", this._updateRuntimes);
    discovery.on("devtools-device-updated", this._updateRuntimes);
    discovery.on("devtools-device-removed", this._updateRuntimes);
    this._updateRuntimes();
  },

  disable() {
    discovery.off("devtools-device-added", this._updateRuntimes);
    discovery.off("devtools-device-updated", this._updateRuntimes);
    discovery.off("devtools-device-removed", this._updateRuntimes);
  },

  _emitUpdated() {
    this.emit("runtime-list-updated");
  },

  _updateRuntimes() {
    this._runtimes = [];
    for (let device of discovery.getRemoteDevicesWithService("devtools")) {
      this._runtimes.push(new WiFiRuntime(device));
    }
    this._emitUpdated();
  },

  scan() {
    discovery.scan();
    return promise.resolve();
  },

  listRuntimes: function() {
    return this._runtimes;
  },

  ALLOWED_PREF: "devtools.remote.wifi.scan",

  get allowed() {
    return Services.prefs.getBoolPref(this.ALLOWED_PREF);
  },

  updateRegistration() {
    if (this.allowed) {
      RuntimeScanners.add(WiFiScanner);
    } else {
      RuntimeScanners.remove(WiFiScanner);
    }
    this._emitUpdated();
  },

  observe(subject, topic, data) {
    if (data !== WiFiScanner.ALLOWED_PREF) {
      return;
    }
    WiFiScanner.updateRegistration();
  }

};

EventEmitter.decorate(WiFiScanner);
WiFiScanner.init();

exports.WiFiScanner = WiFiScanner;

let StaticScanner = {
  enable() {},
  disable() {},
  scan() { return promise.resolve(); },
  listRuntimes() {
    let runtimes = [gRemoteRuntime];
    if (Services.prefs.getBoolPref("devtools.webide.enableLocalRuntime")) {
      runtimes.push(gLocalRuntime);
    }
    return runtimes;
  }
};

EventEmitter.decorate(StaticScanner);
RuntimeScanners.add(StaticScanner);





let RuntimeTypes = exports.RuntimeTypes = {
  USB: "USB",
  WIFI: "WIFI",
  SIMULATOR: "SIMULATOR",
  REMOTE: "REMOTE",
  LOCAL: "LOCAL",
  OTHER: "OTHER"
};







function DeprecatedUSBRuntime(id) {
  this._id = id;
}

DeprecatedUSBRuntime.prototype = {
  type: RuntimeTypes.USB,
  get device() {
    return Devices.getByName(this._id);
  },
  connect: function(connection) {
    if (!this.device) {
      return promise.reject(new Error("Can't find device: " + this.name));
    }
    return this.device.connect().then((port) => {
      connection.host = "localhost";
      connection.port = port;
      connection.connect();
    });
  },
  get id() {
    return this._id;
  },
  get name() {
    return this._productModel || this._id;
  },
  updateNameFromADB: function() {
    if (this._productModel) {
      return promise.reject();
    }
    let deferred = promise.defer();
    if (this.device && this.device.shell) {
      this.device.shell("getprop ro.product.model").then(stdout => {
        this._productModel = stdout;
        deferred.resolve();
      }, () => {});
    } else {
      this._productModel = null;
      deferred.reject();
    }
    return deferred.promise;
  },
};


exports._DeprecatedUSBRuntime = DeprecatedUSBRuntime;

function WiFiRuntime(deviceName) {
  this.deviceName = deviceName;
}

WiFiRuntime.prototype = {
  type: RuntimeTypes.WIFI,
  
  prolongedConnection: true,
  connect: function(connection) {
    let service = discovery.getRemoteService("devtools", this.deviceName);
    if (!service) {
      return promise.reject(new Error("Can't find device: " + this.name));
    }
    connection.advertisement = service;
    connection.authenticator.sendOOB = this.sendOOB;
    
    
    
    connection.timeoutDelay = 0;
    connection.connect();
    return promise.resolve();
  },
  get id() {
    return this.deviceName;
  },
  get name() {
    return this.deviceName;
  },

  
























  sendOOB(session) {
    const WINDOW_ID = "devtools:wifi-auth";
    let { authResult } = session;
    
    if (authResult != AuthenticationResult.PENDING) {
      throw new Error("Expected PENDING result, got " + authResult);
    }

    
    let promptWindow;
    let windowListener = {
      onOpenWindow(xulWindow) {
        let win = xulWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindow);
        win.addEventListener("load", function listener() {
          win.removeEventListener("load", listener, false);
          if (win.document.documentElement.getAttribute("id") != WINDOW_ID) {
            return;
          }
          
          promptWindow = win;
          Services.wm.removeListener(windowListener);
        }, false);
      },
      onCloseWindow() {},
      onWindowTitleChange() {}
    };
    Services.wm.addListener(windowListener);

    
    DevToolsUtils.executeSoon(() => {
      
      
      const MIN_HEIGHT = 600;
      let win = Services.wm.getMostRecentWindow("devtools:webide");
      let width = win.outerWidth * 0.8;
      let height = Math.max(win.outerHeight * 0.5, MIN_HEIGHT);
      win.openDialog("chrome://webide/content/wifi-auth.xhtml",
                     WINDOW_ID,
                     "modal=yes,width=" + width + ",height=" + height, session);
    });

    return {
      close() {
        if (!promptWindow) {
          return;
        }
        promptWindow.close();
        promptWindow = null;
      }
    };
  }
};


exports._WiFiRuntime = WiFiRuntime;

function SimulatorRuntime(simulator) {
  this.simulator = simulator;
}

SimulatorRuntime.prototype = {
  type: RuntimeTypes.SIMULATOR,
  connect: function(connection) {
    return this.simulator.launch().then(port => {
      connection.host = "localhost";
      connection.port = port;
      connection.keepConnecting = true;
      connection.once(Connection.Events.DISCONNECTED, e => this.simulator.kill());
      connection.connect();
    });
  },
  get id() {
    return this.simulator.id;
  },
  get name() {
    return this.simulator.name;
  },
};


exports._SimulatorRuntime = SimulatorRuntime;

let gLocalRuntime = {
  type: RuntimeTypes.LOCAL,
  connect: function(connection) {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
    }
    DebuggerServer.allowChromeProcess = true;
    connection.host = null; 
    connection.port = null;
    connection.connect();
    return promise.resolve();
  },
  get id() {
    return "local";
  },
  get name() {
    return Strings.GetStringFromName("local_runtime");
  },
};


exports._gLocalRuntime = gLocalRuntime;

let gRemoteRuntime = {
  type: RuntimeTypes.REMOTE,
  connect: function(connection) {
    let win = Services.wm.getMostRecentWindow("devtools:webide");
    if (!win) {
      return promise.reject(new Error("No WebIDE window found"));
    }
    let ret = {value: connection.host + ":" + connection.port};
    let title = Strings.GetStringFromName("remote_runtime_promptTitle");
    let message = Strings.GetStringFromName("remote_runtime_promptMessage");
    let ok = Services.prompt.prompt(win, title, message, ret, null, {});
    let [host,port] = ret.value.split(":");
    if (!ok) {
      return promise.reject({canceled: true});
    }
    if (!host || !port) {
      return promise.reject(new Error("Invalid host or port"));
    }
    connection.host = host;
    connection.port = port;
    connection.connect();
    return promise.resolve();
  },
  get name() {
    return Strings.GetStringFromName("remote_runtime");
  },
};


exports._gRemoteRuntime = gRemoteRuntime;
