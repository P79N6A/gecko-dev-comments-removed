



"use strict";





let { Constructor: CC, classes: Cc, interfaces: Ci, utils: Cu } = Components;




let sandbox = Cu.Sandbox(CC('@mozilla.org/systemprincipal;1', 'nsIPrincipal')());
Cu.evalInSandbox(
  "Components.utils.import('resource://gre/modules/jsdebugger.jsm');" +
  "addDebuggerToGlobal(this);",
  sandbox
);
let Debugger = sandbox.Debugger;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil", "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils", "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "console", "resource://gre/modules/devtools/Console.jsm");

let xpcInspector = Cc["@mozilla.org/jsinspector;1"].getService(Ci.nsIJSInspector);

let loader = Cu.import("resource://gre/modules/commonjs/toolkit/loader.js", {}).Loader;
let promise = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;

this.EXPORTED_SYMBOLS = ["DevToolsLoader", "devtools", "BuiltinProvider",
                         "SrcdirProvider"];





let Timer = Cu.import("resource://gre/modules/Timer.jsm", {});

let loaderGlobals = {
  isWorker: false,
  reportError: Cu.reportError,

  btoa: btoa,
  console: console,
  _Iterator: Iterator,
  loader: {
    lazyGetter: XPCOMUtils.defineLazyGetter.bind(XPCOMUtils),
    lazyImporter: XPCOMUtils.defineLazyModuleGetter.bind(XPCOMUtils),
    lazyServiceGetter: XPCOMUtils.defineLazyServiceGetter.bind(XPCOMUtils)
  },
};

let loaderModules = {
  "Debugger": Debugger,
  "Services": Object.create(Services),
  "Timer": Object.create(Timer),
  "toolkit/loader": loader,
  "xpcInspector": xpcInspector,
  "promise": promise,
};
try {
  let { indexedDB } = Cu.Sandbox(this, {wantGlobalProperties:["indexedDB"]});
  loaderModules.indexedDB = indexedDB;
} catch(e) {
  
}

let sharedGlobalBlacklist = ["sdk/indexed-db", "devtools/toolkit/qrcode/decoder/index"];


function BuiltinProvider() {}
BuiltinProvider.prototype = {
  load: function() {
    this.loader = new loader.Loader({
      id: "fx-devtools",
      modules: loaderModules,
      paths: {
        
        
        "": "resource://gre/modules/commonjs/",
        "main": "resource:///modules/devtools/main.js",
        "devtools": "resource:///modules/devtools",
        "devtools/toolkit": "resource://gre/modules/devtools",
        "devtools/server": "resource://gre/modules/devtools/server",
        "devtools/toolkit/webconsole": "resource://gre/modules/devtools/toolkit/webconsole",
        "devtools/app-actor-front": "resource://gre/modules/devtools/app-actor-front.js",
        "devtools/styleinspector/css-logic": "resource://gre/modules/devtools/styleinspector/css-logic",
        "devtools/css-color": "resource://gre/modules/devtools/css-color",
        "devtools/output-parser": "resource://gre/modules/devtools/output-parser",
        "devtools/touch-events": "resource://gre/modules/devtools/touch-events",
        "devtools/client": "resource://gre/modules/devtools/client",
        "devtools/pretty-fast": "resource://gre/modules/devtools/pretty-fast.js",
        "devtools/jsbeautify": "resource://gre/modules/devtools/jsbeautify/beautify.js",
        "devtools/async-utils": "resource://gre/modules/devtools/async-utils",
        "devtools/content-observer": "resource://gre/modules/devtools/content-observer",
        "gcli": "resource://gre/modules/devtools/gcli",
        "projecteditor": "resource:///modules/devtools/projecteditor",
        "acorn": "resource://gre/modules/devtools/acorn",
        "acorn/util/walk": "resource://gre/modules/devtools/acorn/walk.js",
        "tern": "resource://gre/modules/devtools/tern",
        "source-map": "resource://gre/modules/devtools/SourceMap.jsm",

        
        "xpcshell-test": "resource://test"
      },
      globals: loaderGlobals,
      invisibleToDebugger: this.invisibleToDebugger,
      sharedGlobal: true,
      sharedGlobalBlacklist: sharedGlobalBlacklist
    });

    return promise.resolve(undefined);
  },

  unload: function(reason) {
    loader.unload(this.loader, reason);
    delete this.loader;
  },
};




function SrcdirProvider() {}
SrcdirProvider.prototype = {
  fileURI: function(path) {
    let file = new FileUtils.File(path);
    return Services.io.newFileURI(file).spec;
  },

  load: function() {
    let srcdir = Services.prefs.getComplexValue("devtools.loader.srcdir",
                                                Ci.nsISupportsString);
    srcdir = OS.Path.normalize(srcdir.data.trim());
    let devtoolsDir = OS.Path.join(srcdir, "browser", "devtools");
    let toolkitDir = OS.Path.join(srcdir, "toolkit", "devtools");
    let mainURI = this.fileURI(OS.Path.join(devtoolsDir, "main.js"));
    let devtoolsURI = this.fileURI(devtoolsDir);
    let toolkitURI = this.fileURI(toolkitDir);
    let serverURI = this.fileURI(OS.Path.join(toolkitDir, "server"));
    let webconsoleURI = this.fileURI(OS.Path.join(toolkitDir, "webconsole"));
    let appActorURI = this.fileURI(OS.Path.join(toolkitDir, "apps", "app-actor-front.js"));
    let cssLogicURI = this.fileURI(OS.Path.join(toolkitDir, "styleinspector", "css-logic"));
    let cssColorURI = this.fileURI(OS.Path.join(toolkitDir, "css-color"));
    let outputParserURI = this.fileURI(OS.Path.join(toolkitDir, "output-parser"));
    let touchEventsURI = this.fileURI(OS.Path.join(toolkitDir, "touch-events"));
    let clientURI = this.fileURI(OS.Path.join(toolkitDir, "client"));
    let prettyFastURI = this.fileURI(OS.Path.join(toolkitDir), "pretty-fast.js");
    let jsBeautifyURI = this.fileURI(OS.Path.join(toolkitDir, "jsbeautify", "beautify.js"));
    let asyncUtilsURI = this.fileURI(OS.Path.join(toolkitDir), "async-utils.js");
    let contentObserverURI = this.fileURI(OS.Path.join(toolkitDir), "content-observer.js");
    let gcliURI = this.fileURI(OS.Path.join(toolkitDir, "gcli", "source", "lib", "gcli"));
    let projecteditorURI = this.fileURI(OS.Path.join(devtoolsDir, "projecteditor"));
    let acornURI = this.fileURI(OS.Path.join(toolkitDir, "acorn"));
    let acornWalkURI = OS.Path.join(acornURI, "walk.js");
    let ternURI = OS.Path.join(toolkitDir, "tern");
    let sourceMapURI = this.fileURI(OS.Path.join(toolkitDir), "SourceMap.jsm");
    this.loader = new loader.Loader({
      id: "fx-devtools",
      modules: loaderModules,
      paths: {
        "": "resource://gre/modules/commonjs/",
        "main": mainURI,
        "devtools": devtoolsURI,
        "devtools/toolkit": toolkitURI,
        "devtools/server": serverURI,
        "devtools/toolkit/webconsole": webconsoleURI,
        "devtools/app-actor-front": appActorURI,
        "devtools/styleinspector/css-logic": cssLogicURI,
        "devtools/css-color": cssColorURI,
        "devtools/output-parser": outputParserURI,
        "devtools/touch-events": touchEventsURI,
        "devtools/client": clientURI,
        "devtools/pretty-fast": prettyFastURI,
        "devtools/jsbeautify": jsBeautifyURI,
        "devtools/async-utils": asyncUtilsURI,
        "devtools/content-observer": contentObserverURI,
        "gcli": gcliURI,
        "projecteditor": projecteditorURI,
        "acorn": acornURI,
        "acorn/util/walk": acornWalkURI,
        "tern": ternURI,
        "source-map": sourceMapURI,
      },
      globals: loaderGlobals,
      invisibleToDebugger: this.invisibleToDebugger,
      sharedGlobal: true,
      sharedGlobalBlacklist: sharedGlobalBlacklist
    });

    return this._writeManifest(devtoolsDir).then(null, Cu.reportError);
  },

  unload: function(reason) {
    loader.unload(this.loader, reason);
    delete this.loader;
  },

  _readFile: function(filename) {
    let deferred = promise.defer();
    let file = new FileUtils.File(filename);
    NetUtil.asyncFetch(file, (inputStream, status) => {
      if (!Components.isSuccessCode(status)) {
        deferred.reject(new Error("Couldn't load manifest: " + filename + "\n"));
        return;
      }
      var data = NetUtil.readInputStreamToString(inputStream, inputStream.available());
      deferred.resolve(data);
    });
    return deferred.promise;
  },

  _writeFile: function(filename, data) {
    let deferred = promise.defer();
    let file = new FileUtils.File(filename);

    var ostream = FileUtils.openSafeFileOutputStream(file)

    var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    var istream = converter.convertToInputStream(data);
    NetUtil.asyncCopy(istream, ostream, (status) => {
      if (!Components.isSuccessCode(status)) {
        deferred.reject(new Error("Couldn't write manifest: " + filename + "\n"));
        return;
      }

      deferred.resolve(null);
    });
    return deferred.promise;
  },

  _writeManifest: function(dir) {
    return this._readFile(OS.Path.join(dir, "jar.mn")).then((data) => {
      
      
      let entries = [];
      let lines = data.split(/\n/);
      let preprocessed = /^\s*\*/;
      let contentEntry = new RegExp("^\\s+content/(\\w+)/(\\S+)\\s+\\((\\S+)\\)");
      for (let line of lines) {
        if (preprocessed.test(line)) {
          dump("Unable to override preprocessed file: " + line + "\n");
          continue;
        }
        let match = contentEntry.exec(line);
        if (match) {
          let pathComponents = match[3].split("/");
          pathComponents.unshift(dir);
          let path = OS.Path.join.apply(OS.Path, pathComponents);
          let uri = this.fileURI(path);
          let entry = "override chrome://" + match[1] + "/content/" + match[2] + "\t" + uri;
          entries.push(entry);
        }
      }
      return this._writeFile(OS.Path.join(dir, "chrome.manifest"), entries.join("\n"));
    }).then(() => {
      Components.manager.addBootstrappedManifestLocation(new FileUtils.File(dir));
    });
  }
};








this.DevToolsLoader = function DevToolsLoader() {
  this.require = this.require.bind(this);
  this.lazyRequireGetter = this.lazyRequireGetter.bind(this);
};

DevToolsLoader.prototype = {
  get provider() {
    if (!this._provider) {
      this._chooseProvider();
    }
    return this._provider;
  },

  _provider: null,

  




  require: function() {
    this._chooseProvider();
    return this.require.apply(this, arguments);
  },

  











  lazyRequireGetter: function (obj, property, module) {
    Object.defineProperty(obj, property, {
      get: () => this.require(module)
    });
  },

  







  loadURI: function(id, uri) {
    let module = loader.Module(id, uri);
    return loader.load(this.provider.loader, module).exports;
  },

  








  main: function(id) {
    
    
    if (this._mainid) {
      return;
    }
    this._mainid = id;
    this._main = loader.main(this.provider.loader, id);

    
    Object.getOwnPropertyNames(this._main).forEach(key => {
      XPCOMUtils.defineLazyGetter(this, key, () => this._main[key]);
    });
  },

  


  setProvider: function(provider) {
    if (provider === this._provider) {
      return;
    }

    if (this._provider) {
      var events = this.require("sdk/system/events");
      events.emit("devtools-unloaded", {});
      delete this.require;
      this._provider.unload("newprovider");
    }
    this._provider = provider;
    this._provider.invisibleToDebugger = this.invisibleToDebugger;
    this._provider.load();
    this.require = loader.Require(this._provider.loader, { id: "devtools" });

    if (this._mainid) {
      this.main(this._mainid);
    }
  },

  


  _chooseProvider: function() {
    if (Services.prefs.prefHasUserValue("devtools.loader.srcdir")) {
      this.setProvider(new SrcdirProvider());
    } else {
      this.setProvider(new BuiltinProvider());
    }
  },

  


  reload: function() {
    var events = this.require("sdk/system/events");
    events.emit("startupcache-invalidate", {});
    events.emit("devtools-unloaded", {});

    this._provider.unload("reload");
    delete this._provider;
    this._chooseProvider();
  },

  








  invisibleToDebugger: Services.appinfo.name !== "Firefox"
};


this.devtools = new DevToolsLoader();
