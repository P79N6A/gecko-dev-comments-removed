




'use strict';

const { Cc, Ci, Cu } = require("chrome");

const Environment = require("sdk/system/environment").env;
const EventEmitter = require("devtools/toolkit/event-emitter");
const promise = require("promise");
const Subprocess = require("sdk/system/child_process/subprocess");
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});

loader.lazyGetter(this, "OS", () => {
  const Runtime = require("sdk/system/runtime");
  switch (Runtime.OS) {
    case "Darwin":
      return "mac64";
    case "Linux":
      if (Runtime.XPCOMABI.indexOf("x86_64") === 0) {
        return "linux64";
      } else {
        return "linux32";
      }
    case "WINNT":
      return "win32";
    default:
      return "";
  }
});

function SimulatorProcess() {}
SimulatorProcess.prototype = {

  
  get isRunning() !!this.process,

  
  run() {

    
    let b2g = this.b2gBinary;
    if (!b2g || !b2g.exists()) {
      throw Error("B2G executable not found.");
    }

    
    let gaia = this.gaiaProfile;
    if (!gaia || !gaia.exists()) {
      throw Error("Gaia profile directory not found.");
    }

    this.once("stdout", function () {
      if (OS == "mac64") {
        console.debug("WORKAROUND run osascript to show b2g-desktop window on OS=='mac64'");
        
        let path = b2g.path.replace(/\\/g, "\\\\").replace(/\"/g, '\\"');

        Subprocess.call({
          command: "/usr/bin/osascript",
          arguments: ["-e", 'tell application "' + path + '" to activate'],
        });
      }
    });

    this.on("stdout", (e, data) => this.log(e, data.trim()));
    this.on("stderr", (e, data) => this.log(e, data.trim()));

    let environment;
    if (OS.indexOf("linux") > -1) {
      environment = ["TMPDIR=" + Services.dirsvc.get("TmpD", Ci.nsIFile).path];
      if ("DISPLAY" in Environment) {
        environment.push("DISPLAY=" + Environment.DISPLAY);
      }
    }

    
    this.process = Subprocess.call({
      command: b2g,
      arguments: this.args,
      environment: environment,
      stdout: data => this.emit("stdout", data),
      stderr: data => this.emit("stderr", data),
      
      
      done: result => {
        console.log("B2G terminated with " + result.exitCode);
        this.process = null;
        this.emit("exit", result.exitCode);
      }
    });
  },

  
  kill() {
    let deferred = promise.defer();
    if (this.process) {
      this.once("exit", (e, exitCode) => {
        this.shuttingDown = false;
        deferred.resolve(exitCode);
      });
      if (!this.shuttingDown) {
        this.shuttingDown = true;
        this.emit("kill", null);
        this.process.kill();
      }
      return deferred.promise;
    } else {
      return promise.resolve(undefined);
    }
  },

  
  log(level, message) {
    if (!Services.prefs.getBoolPref("devtools.webide.logSimulatorOutput")) {
      return;
    }
    if (level === "stderr" || level === "error") {
      console.error(message);
      return;
    }
    console.log(message);
  },

  
  get args() {
    let args = [];

    
    args.push("-profile", this.gaiaProfile.path);

    
    let port = parseInt(this.options.port);
    args.push("-start-debugger-server", "" + port);

    
    let width = parseInt(this.options.width);
    let height = parseInt(this.options.height);
    if (width && height) {
      args.push("-screen", width + "x" + height);
    }

    
    args.push("-no-remote");

    return args;
  },
};

EventEmitter.decorate(SimulatorProcess.prototype);


function CustomSimulatorProcess(options) {
  this.options = options;
}

let CSPp = CustomSimulatorProcess.prototype = Object.create(SimulatorProcess.prototype);


Object.defineProperty(CSPp, "b2gBinary", {
  get: function() {
    let file = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
    file.initWithPath(this.options.b2gBinary);
    return file;
  }
});


Object.defineProperty(CSPp, "gaiaProfile", {
  get: function() {
    let file = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
    file.initWithPath(this.options.gaiaProfile);
    return file;
  }
});

exports.CustomSimulatorProcess = CustomSimulatorProcess;


function AddonSimulatorProcess(addon, options) {
  this.addon = addon;
  this.options = options;
}

let ASPp = AddonSimulatorProcess.prototype = Object.create(SimulatorProcess.prototype);


Object.defineProperty(ASPp, "b2gBinary", {
  get: function() {
    let file;
    try {
      let pref = "extensions." + this.addon.id + ".customRuntime";
      file = Services.prefs.getComplexValue(pref, Ci.nsIFile);
    } catch(e) {}

    if (!file) {
      file = this.addon.getResourceURI().QueryInterface(Ci.nsIFileURL).file;
      file.append("b2g");
      let binaries = {
        win32: "b2g-bin.exe",
        mac64: "B2G.app/Contents/MacOS/b2g-bin",
        linux32: "b2g-bin",
        linux64: "b2g-bin",
      };
      binaries[OS].split("/").forEach(node => file.append(node));
    }
    return file;
  }
});


Object.defineProperty(ASPp, "gaiaProfile", {
  get: function() {
    let file;

    
    if (this.options.gaiaProfile) {
      file = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
      file.initWithPath(this.options.gaiaProfile);
      return file;
    }

    
    try {
      let pref = "extensions." + this.addon.id + ".gaiaProfile";
      file = Services.prefs.getComplexValue(pref, Ci.nsIFile);
      return file;
    } catch(e) {}

    
    file = this.addon.getResourceURI().QueryInterface(Ci.nsIFileURL).file;
    file.append("profile");
    return file;
  }
});

exports.AddonSimulatorProcess = AddonSimulatorProcess;


function OldAddonSimulatorProcess(addon, options) {
  this.addon = addon;
  this.options = options;
}

let OASPp = OldAddonSimulatorProcess.prototype = Object.create(AddonSimulatorProcess.prototype);


Object.defineProperty(OASPp, "b2gBinary", {
  get: function() {
    let file;
    try {
      let pref = "extensions." + this.addon.id + ".customRuntime";
      file = Services.prefs.getComplexValue(pref, Ci.nsIFile);
    } catch(e) {}

    if (!file) {
      file = this.addon.getResourceURI().QueryInterface(Ci.nsIFileURL).file;
      let version = this.addon.name.match(/\d+\.\d+/)[0].replace(/\./, "_");
      file.append("resources");
      file.append("fxos_" + version + "_simulator");
      file.append("data");
      file.append(OS == "linux32" ? "linux" : OS);
      let binaries = {
        win32: "b2g/b2g-bin.exe",
        mac64: "B2G.app/Contents/MacOS/b2g-bin",
        linux32: "b2g/b2g-bin",
        linux64: "b2g/b2g-bin",
      };
      binaries[OS].split("/").forEach(node => file.append(node));
    }
    return file;
  }
});


Object.defineProperty(OASPp, "args", {
  get: function() {
    let args = [];

    
    args.push("-profile", this.gaiaProfile.path);

    
    let port = parseInt(this.options.port);
    args.push("-dbgport", "" + port);

    
    args.push("-no-remote");

    return args;
  }
});

exports.OldAddonSimulatorProcess = OldAddonSimulatorProcess;
