




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const DBG_XUL = "chrome://browser/content/devtools/framework/toolbox-process-window.xul";
const CHROME_DEBUGGER_PROFILE_NAME = "-chrome-debugger";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm")

XPCOMUtils.defineLazyModuleGetter(this, "DevToolsLoader",
  "resource://gre/modules/devtools/Loader.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "devtools",
  "resource://gre/modules/devtools/Loader.jsm");

XPCOMUtils.defineLazyGetter(this, "Telemetry", function () {
  return devtools.require("devtools/shared/telemetry");
});
XPCOMUtils.defineLazyGetter(this, "EventEmitter", function () {
  return devtools.require("devtools/toolkit/event-emitter");
});
const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});

this.EXPORTED_SYMBOLS = ["BrowserToolboxProcess"];

let processes = Set();











this.BrowserToolboxProcess = function BrowserToolboxProcess(aOnClose, aOnRun, aOptions) {
  let emitter = new EventEmitter();
  this.on = emitter.on.bind(emitter);
  this.off = emitter.off.bind(emitter);
  this.once = emitter.once.bind(emitter);
  
  this.emit = function(...args) {
    emitter.emit(...args);
    BrowserToolboxProcess.emit(...args);
  }

  
  
  if (typeof aOnClose === "object") {
    if (aOnClose.onClose) {
      this.on("close", aOnClose.onClose);
    }
    if (aOnClose.onRun) {
      this.on("run", aOnClose.onRun);
    }
    this._options = aOnClose;
  } else {
    if (aOnClose) {
      this.on("close", aOnClose);
    }
    if (aOnRun) {
      this.on("run", aOnRun);
    }
    this._options = aOptions || {};
  }

  this._telemetry = new Telemetry();

  this.close = this.close.bind(this);
  Services.obs.addObserver(this.close, "quit-application", false);
  this._initServer();
  this._initProfile();
  this._create();

  processes.add(this);
};

EventEmitter.decorate(BrowserToolboxProcess);





BrowserToolboxProcess.init = function(aOnClose, aOnRun, aOptions) {
  return new BrowserToolboxProcess(aOnClose, aOnRun, aOptions);
};










BrowserToolboxProcess.setAddonOptions = function DSC_setAddonOptions(aId, aOptions) {
  let promises = [];

  for (let process of processes.values()) {
    promises.push(process.debuggerServer.setAddonOptions(aId, aOptions));
  }

  return promise.all(promises);
};

BrowserToolboxProcess.prototype = {
  


  _initServer: function() {
    dumpn("Initializing the chrome toolbox server.");

    if (!this.loader) {
      
      
      
      
      
      this.loader = new DevToolsLoader();
      this.loader.invisibleToDebugger = true;
      this.loader.main("devtools/server/main");
      this.debuggerServer = this.loader.DebuggerServer;
      dumpn("Created a separate loader instance for the DebuggerServer.");

      
      this.debuggerServer.on("connectionchange", this.emit.bind(this));
    }

    if (!this.debuggerServer.initialized) {
      this.debuggerServer.init();
      this.debuggerServer.addBrowserActors();
      dumpn("initialized and added the browser actors for the DebuggerServer.");
    }

    let chromeDebuggingPort =
      Services.prefs.getIntPref("devtools.debugger.chrome-debugging-port");
    this.debuggerServer.openListener(chromeDebuggingPort);

    dumpn("Finished initializing the chrome toolbox server.");
    dumpn("Started listening on port: " + chromeDebuggingPort);
  },

  


  _initProfile: function() {
    dumpn("Initializing the chrome toolbox user profile.");

    let profileService = Cc["@mozilla.org/toolkit/profile-service;1"]
      .createInstance(Ci.nsIToolkitProfileService);

    let profileName;
    try {
      
      profileName = profileService.selectedProfile.name + CHROME_DEBUGGER_PROFILE_NAME;
      dumpn("Using chrome toolbox profile name: " + profileName);
    } catch (e) {
      
      profileName = CHROME_DEBUGGER_PROFILE_NAME;
      let msg = "Querying the current profile failed. " + e.name + ": " + e.message;
      dumpn(msg);
      Cu.reportError(msg);
    }

    let profileObject;
    try {
      
      profileObject = profileService.getProfileByName(profileName);
      dumpn("Using chrome toolbox profile object: " + profileObject);

      
      var enumerator = Services.dirsvc.get("ProfD", Ci.nsIFile).parent.directoryEntries;
      while (enumerator.hasMoreElements()) {
        let profileDir = enumerator.getNext().QueryInterface(Ci.nsIFile);
        if (profileDir.leafName.contains(profileName)) {
          
          this._dbgProfile = profileObject;
          return;
        }
      }
      
      profileObject.remove(true);
      dumpn("The already existing chrome toolbox profile was invalid.");
    } catch (e) {
      
      let msg = "Creating a profile failed. " + e.name + ": " + e.message;
      dumpn(msg);
      Cu.reportError(msg);
    }

    
    this._dbgProfile = profileService.createProfile(null, profileName);
    profileService.flush();

    dumpn("Finished creating the chrome toolbox user profile.");
    dumpn("Flushed profile service with: " + profileName);
  },

  


  _create: function() {
    dumpn("Initializing chrome debugging process.");
    let process = this._dbgProcess = Cc["@mozilla.org/process/util;1"].createInstance(Ci.nsIProcess);
    process.init(Services.dirsvc.get("XREExeF", Ci.nsIFile));

    let xulURI = DBG_XUL;

    if (this._options.addonID) {
      xulURI += "?addonID=" + this._options.addonID;
    }

    dumpn("Running chrome debugging process.");
    let args = ["-no-remote", "-foreground", "-P", this._dbgProfile.name, "-chrome", xulURI];

    process.runwAsync(args, args.length, { observe: () => this.close() });

    this._telemetry.toolOpened("jsbrowserdebugger");

    dumpn("Chrome toolbox is now running...");
    this.emit("run", this);
  },

  


  close: function() {
    if (this.closed) {
      return;
    }

    dumpn("Cleaning up the chrome debugging process.");
    Services.obs.removeObserver(this.close, "quit-application");

    if (this._dbgProcess.isRunning) {
      this._dbgProcess.kill();
    }

    this._telemetry.toolClosed("jsbrowserdebugger");
    if (this.debuggerServer) {
      this.debuggerServer.destroy();
    }

    dumpn("Chrome toolbox is now closed...");
    this.closed = true;
    this.emit("close", this);
    processes.delete(this);
  }
};





function dumpn(str) {
  if (wantLogging) {
    dump("DBG-FRONTEND: " + str + "\n");
  }
}

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");

Services.prefs.addObserver("devtools.debugger.log", {
  observe: (...args) => wantLogging = Services.prefs.getBoolPref(args.pop())
}, false);

Services.obs.notifyObservers(null, "ToolboxProcessLoaded", null);
