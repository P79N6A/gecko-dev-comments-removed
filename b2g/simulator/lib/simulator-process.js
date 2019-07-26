




'use strict';

const { Cc, Ci, Cu, ChromeWorker } = require("chrome");

Cu.import("resource://gre/modules/Services.jsm");

const { EventTarget } = require("sdk/event/target");
const { emit, off } = require("sdk/event/core");
const { Class } = require("sdk/core/heritage");
const Environment = require("sdk/system/environment").env;
const Runtime = require("sdk/system/runtime");
const Self = require("sdk/self");
const URL = require("sdk/url");
const Subprocess = require("subprocess");
const { Promise } = Cu.import("resource://gre/modules/Promise.jsm", {});

const { rootURI: ROOT_URI } = require('@loader/options');
const PROFILE_URL = ROOT_URI + "profile/";
const BIN_URL = ROOT_URI + "b2g/";





Subprocess.registerLogHandler(
  function(s) console.error("subprocess: " + s.trim())
);
Subprocess.registerDebugHandler(
  function(s) console.debug("subprocess: " + s.trim())
);

exports.SimulatorProcess = Class({
  extends: EventTarget,
  initialize: function initialize(options) {
    EventTarget.prototype.initialize.call(this, options);

    this.on("stdout", function onStdout(data) console.log(data.trim()));
    this.on("stderr", function onStderr(data) console.error(data.trim()));
  },

  
  get isRunning() !!this.process,

  


  run: function() {
    
    if (this.process != null) {
      this.process
          .kill()
          .then(this.run.bind(this));
      return;
    }

    
    let b2gExecutable = this.b2gExecutable;

    this.once("stdout", function () {
      if (Runtime.OS == "Darwin") {
          console.debug("WORKAROUND run osascript to show b2g-desktop window"+
                        " on Runtime.OS=='Darwin'");
        
        let path = b2gExecutable.path
          .replace(/\\/g, "\\\\").replace(/\"/g, '\\"');

        Subprocess.call({
          command: "/usr/bin/osascript",
          arguments: ["-e", 'tell application "' + path + '" to activate'],
        });
      }
    });

    let environment;
    if (Runtime.OS == "Linux") {
      environment = ["TMPDIR=" + Services.dirsvc.get("TmpD",Ci.nsIFile).path];
      if ("DISPLAY" in Environment) {
        environment.push("DISPLAY=" + Environment.DISPLAY);
      }
    }

    
    this.process = Subprocess.call({
      command: b2gExecutable,
      arguments: this.b2gArguments,
      environment: environment,

      
      stdout: (function(data) {
        emit(this, "stdout", data);
      }).bind(this),

      
      stderr: (function(data) {
        emit(this, "stderr", data);
      }).bind(this),

      
      
      done: (function(result) {
        console.log(this.b2gFilename + " terminated with " + result.exitCode);
        this.process = null;
        emit(this, "exit", result.exitCode);
      }).bind(this)
    });
  },

  
  kill: function() {
    let deferred = Promise.defer();
    if (this.process) {
      this.once("exit", (exitCode) => {
        this.shuttingDown = false;
        deferred.resolve(exitCode);
      });
      if (!this.shuttingDown) {
        this.shuttingDown = true;
        emit(this, "kill", null);
        this.process.kill();
      }
      return deferred.promise;
    } else {
      return Promise.resolve(undefined);
    }
  },

  
  get b2gFilename() {
    return this._executable ? this._executableFilename : "B2G";
  },

  
  get b2gExecutable() {
    if (this._executable) return this._executable;

    let bin = URL.toFilename(BIN_URL);
    let executables = {
      WINNT: "b2g-bin.exe",
      Darwin: "Contents/MacOS/b2g-bin",
      Linux: "b2g-bin",
    };

    console.log("bin url: "+bin+"/"+executables[Runtime.OS]);
    let path = bin + "/" + executables[Runtime.OS];

    let executable = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    executable.initWithPath(path);

    if (!executable.exists()) {
      
      throw Error("b2g-desktop Executable not found.");
    }

    this._executable = executable;
    this._executableFilename = "b2g-bin";

    return executable;
  },

  
  get b2gArguments() {
    let args = [];

    let profile = URL.toFilename(PROFILE_URL);
    args.push("-profile", profile);
    Cu.reportError(profile);

    
    args.push("-start-debugger-server", "" + this.remoteDebuggerPort);

    
    args.push("-no-remote");

    return args;
  },
});

