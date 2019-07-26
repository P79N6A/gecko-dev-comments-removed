




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const DBG_XUL = "chrome://browser/content/devtools/debugger.xul";
const CHROME_DEBUGGER_PROFILE_NAME = "-chrome-debugger";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

Cu.import("resource://gre/modules/devtools/Loader.jsm");
let require = devtools.require;
let Telemetry = require("devtools/shared/telemetry");

this.EXPORTED_SYMBOLS = ["BrowserDebuggerProcess"];









this.BrowserDebuggerProcess = function BrowserDebuggerProcess(aOnClose, aOnRun) {
  this._closeCallback = aOnClose;
  this._runCallback = aOnRun;
  this._telemetry = new Telemetry();

  this._initServer();
  this._initProfile();
  this._create();
}





BrowserDebuggerProcess.init = function(aOnClose, aOnRun) {
  return new BrowserDebuggerProcess(aOnClose, aOnRun);
};

BrowserDebuggerProcess.prototype = {
  


  _initServer: function() {
    if (!this.loader) {
      
      
      
      
      this.loader = new DevToolsLoader();
      this.loader.main("devtools/server/main");
      this.debuggerServer = this.loader.DebuggerServer;
    }

    if (!this.debuggerServer.initialized) {
      this.debuggerServer.init();
      this.debuggerServer.addBrowserActors();
    }
    this.debuggerServer.openListener(Prefs.chromeDebuggingPort);
  },

  


  _initProfile: function() {
    let profileService = Cc["@mozilla.org/toolkit/profile-service;1"]
      .createInstance(Ci.nsIToolkitProfileService);

    let profileName;
    try {
      
      profileName = profileService.selectedProfile.name + CHROME_DEBUGGER_PROFILE_NAME;
    } catch (e) {
      
      profileName = CHROME_DEBUGGER_PROFILE_NAME;
      let msg = "Querying the current profile failed. " + e.name + ": " + e.message;
      dumpn(msg);
      Cu.reportError(msg);
    }

    let profileObject;
    try {
      
      profileObject = profileService.getProfileByName(profileName);

      
      var enumerator = Services.dirsvc.get("ProfD", Ci.nsIFile).parent.directoryEntries;
      while (enumerator.hasMoreElements()) {
        let profileDir = enumerator.getNext().QueryInterface(Ci.nsIFile);
        if (profileDir.leafName.contains(profileName)) {
          
          this._dbgProfile = profileObject;
          return;
        }
      }
      
      profileObject.remove(true);
    } catch (e) {
      
      let msg = "Creating a profile failed. " + e.name + ": " + e.message;
      dumpn(msg);
      Cu.reportError(msg);
    }

    
    this._dbgProfile = profileService.createProfile(null, null, profileName);
    profileService.flush();
  },

  


  _create: function() {
    dumpn("Initializing chrome debugging process.");
    let process = this._dbgProcess = Cc["@mozilla.org/process/util;1"].createInstance(Ci.nsIProcess);
    process.init(Services.dirsvc.get("XREExeF", Ci.nsIFile));

    dumpn("Running chrome debugging process.");
    let args = ["-no-remote", "-foreground", "-P", this._dbgProfile.name, "-chrome", DBG_XUL];
    process.runwAsync(args, args.length, { observe: () => this.close() });

    this._telemetry.toolOpened("jsbrowserdebugger");

    dumpn("Chrome debugger is now running...");
    if (typeof this._runCallback == "function") {
      this._runCallback.call({}, this);
    }
  },

  


  close: function() {
    if (this._dbgProcess.isRunning) {
      dumpn("Killing chrome debugging process...");
      this._dbgProcess.kill();
    }

    this._telemetry.toolClosed("jsbrowserdebugger");

    this.debuggerServer.destroy();

    dumpn("Chrome debugger is now closed...");
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
