




'use strict';

const { Cc, Ci, Cu, ChromeWorker } = require("chrome");

Cu.import("resource://gre/modules/Services.jsm");

const Environment = require("sdk/system/environment").env;
const Runtime = require("sdk/system/runtime");
const Subprocess = require("sdk/system/child_process/subprocess");
const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
const { EventEmitter } = Cu.import("resource://gre/modules/devtools/event-emitter.js", {});






Subprocess.registerLogHandler(
  function(s) console.error("subprocess: " + s.trim())
);
Subprocess.registerDebugHandler(
  function(s) console.debug("subprocess: " + s.trim())
);

function SimulatorProcess(options) {
  this.options = options;

  EventEmitter.decorate(this);
  this.on("stdout", data => { console.log(data.trim()) });
  this.on("stderr", data => { console.error(data.trim()) });
}

SimulatorProcess.prototype = {

  
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
      environment = ["TMPDIR=" + Services.dirsvc.get("TmpD", Ci.nsIFile).path];
      if ("DISPLAY" in Environment) {
        environment.push("DISPLAY=" + Environment.DISPLAY);
      }
    }

    
    this.process = Subprocess.call({
      command: b2gExecutable,
      arguments: this.b2gArguments,
      environment: environment,

      
      stdout: data => {
        this.emit("stdout", data);
      },

      
      stderr: data => {
        this.emit("stderr", data);
      },

      
      
      done: (function(result) {
        console.log("B2G terminated with " + result.exitCode);
        this.process = null;
        this.emit("exit", result.exitCode);
      }).bind(this)
    });
  },

  
  kill: function() {
    let deferred = promise.defer();
    if (this.process) {
      this.once("exit", (exitCode) => {
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

  
  get b2gExecutable() {
    if (this._executable) {
      return this._executable;
    }

    let executable = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    executable.initWithPath(this.options.runtimePath);

    if (!executable.exists()) {
      
      throw Error("b2g-desktop Executable not found.");
    }

    this._executable = executable;

    return executable;
  },

  
  get b2gArguments() {
    let args = [];

    let profile = this.options.profilePath;
    args.push("-profile", profile);
    console.log("profile", profile);

    
    args.push("-start-debugger-server", "" + this.options.port);

    
    args.push("-no-remote");

    return args;
  },
};

exports.SimulatorProcess = SimulatorProcess;
