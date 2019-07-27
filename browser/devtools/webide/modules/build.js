



const {Cu, Cc, Ci} = require("chrome");

const promise = require("promise");
const { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
const { TextDecoder, OS }  = Cu.import("resource://gre/modules/osfile.jsm", {});
const Subprocess = require("sdk/system/child_process/subprocess");

const ProjectBuilding = exports.ProjectBuilding = {
  fetchPackageManifest: Task.async(function * (project) {
    let manifestPath = OS.Path.join(project.location, "package.json");
    let exists = yield OS.File.exists(manifestPath);
    if (!exists) {
      return;
    }

    let Decoder = new TextDecoder();
    let data = yield OS.File.read(manifestPath);
    data = new TextDecoder().decode(data);
    let manifest;
    try {
      manifest = JSON.parse(data);
    } catch(e) {
      throw new Error("Error while reading WebIDE manifest at: '" + manifestPath +
                      "', invalid JSON: " + e.message);
    }
    return manifest;
  }),

  hasPrepackage: Task.async(function* (project) {
    let manifest = yield ProjectBuilding.fetchPackageManifest(project);
    return manifest && manifest.webide && "prepackage" in manifest.webide;
  }),

  
  build: Task.async(function* ({ project, logger }) {
    if (!(yield this.hasPrepackage(project))) {
      return;
    }

    let manifest = yield ProjectBuilding.fetchPackageManifest(project);

    logger("start");
    let packageDir;
    try {
      packageDir = yield this._build(project, manifest, logger);
      logger("succeed");
    } catch(e) {
      logger("failed", e);
    }

    return packageDir;
  }),

  _build: Task.async(function* (project, manifest, logger) {
    
    manifest = manifest.webide;

    let command, cwd, args = [], env = [];

    if (typeof(manifest.prepackage) === "string") {
      command = manifest.prepackage.replace(/%project%/g, project.location);
    } else if (manifest.prepackage.command) {
      command = manifest.prepackage.command;

      args = manifest.prepackage.args || [];
      args = args.map(a => a.replace(/%project%/g, project.location));

      env = manifest.prepackage.env || [];
      env = env.map(a => a.replace(/%project%/g, project.location));

      
      let envService = Cc["@mozilla.org/process/environment;1"].getService(Ci.nsIEnvironment);
      let home = envService.get("HOME");
      if (home) {
        env.push("HOME=" + home);
      }

      if (manifest.prepackage.cwd) {
        
        let path = OS.Path.normalize(manifest.prepackage.cwd);
        
        
        let rel = OS.Path.join(project.location, path);
        let exists = yield OS.File.exists(rel);
        if (exists) {
          cwd = rel;
        }
      }
    } else {
      throw new Error("pre-package manifest is invalid, missing or invalid " +
                      "`prepackage` attribute");
    }

    if (!cwd) {
      cwd = project.location;
    }

    logger("Running pre-package hook '" + command + "' " +
           args.join(" ") +
           " with ENV=[" + env.join(", ") + "]" +
           " at " + cwd);

    
    
    
    
    
    
    let envService = Cc["@mozilla.org/process/environment;1"].getService(Ci.nsIEnvironment);
    let shell = envService.get("ComSpec") || envService.get("SHELL");
    args.unshift(command);

    
    
    
    if (envService.exists("ComSpec")) {
      args.unshift("/C");
    } else {
      args.unshift("-c");
    }

    
    let originalCwd = yield OS.File.getCurrentDirectory();
    try {
      let defer = promise.defer();
      Subprocess.call({
        command: shell,
        arguments: args,
        environment: env,
        workdir: cwd,

        stdout: data =>
          logger(data),
        stderr: data =>
          logger(data),

        done: result => {
          logger("Terminated with error code: " + result.exitCode);
          if (result.exitCode == 0) {
            defer.resolve();
          } else {
            defer.reject("pre-package command failed with error code " + result.exitCode);
          }
        }
      });
      defer.promise.then(() => {
        OS.File.setCurrentDirectory(originalCwd);
      });
      yield defer.promise;
    } catch (e) {
      throw new Error("Unable to run pre-package command '" + command + "' " +
                      args.join(" ") + ":\n" + (e.message || e));
    }

    if (manifest.packageDir) {
      let packageDir = OS.Path.join(project.location, manifest.packageDir);
      
      packageDir = OS.Path.normalize(packageDir);
      let exists = yield OS.File.exists(packageDir);
      if (exists) {
        return packageDir;
      }
      throw new Error("Unable to resolve application package directory: '" + manifest.packageDir + "'");
    }
  }),
};
