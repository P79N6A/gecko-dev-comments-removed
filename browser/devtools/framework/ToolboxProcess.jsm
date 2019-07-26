




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const DBG_XUL = "chrome://browser/content/devtools/framework/toolbox-process-window.xul";
const CHROME_DEBUGGER_PROFILE_NAME = "-chrome-debugger";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

Cu.import("resource://gre/modules/devtools/Loader.jsm");
let require = devtools.require;
let Telemetry = require("devtools/shared/telemetry");

this.EXPORTED_SYMBOLS = ["BrowserToolboxProcess"];











this.BrowserToolboxProcess = function BrowserToolboxProcess(aOnClose, aOnRun, aOptions) {
  
  
  if (typeof aOnClose === "object") {
    this._closeCallback = aOnClose.onClose;
    this._runCallback = aOnClose.onRun;
    this._options = aOnClose;
  } else {
    this._closeCallback = aOnClose;
    this._runCallback = aOnRun;
    this._options = aOptions || {};
  }

  this._telemetry = new Telemetry();

  this.close = this.close.bind(this);
  Services.obs.addObserver(this.close, "quit-application", false);
  this._initServer();
  this._initProfile();
  this._create();
};





BrowserToolboxProcess.init = function(aOnClose, aOnRun, aOptions) {
  return new BrowserToolboxProcess(aOnClose, aOnRun, aOptions);
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
    }

    if (!this.debuggerServer.initialized) {
      this.debuggerServer.init();
      this.debuggerServer.addBrowserActors();
      dumpn("initialized and added the browser actors for the DebuggerServer.");
    }

    this.debuggerServer.openListener(Prefs.chromeDebuggingPort);

    dumpn("Finished initializing the chrome toolbox server.");
    dumpn("Started listening on port: " + Prefs.chromeDebuggingPort);
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
    if (typeof this._runCallback == "function") {
      this._runCallback.call({}, this);
    }
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
    if (typeof this._closeCallback == "function") {
      this._closeCallback.call({}, this);
    }
  }
};




let Prefs = new ViewHelpers.Prefs("devtools.debugger", {
  chromeDebuggingHost: ["Char", "chrome-debugging-host"],
  chromeDebuggingPort: ["Int", "chrome-debugging-port"]
});





function dumpn(str) {
  if (wantLogging) {
    dump("DBG-FRONTEND: " + str + "\n");
  }
}

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");

Services.prefs.addObserver("devtools.debugger.log", {
  observe: (...args) => wantLogging = Services.prefs.getBoolPref(args.pop())
}, false);
