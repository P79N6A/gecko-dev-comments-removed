



"use strict";





let { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil", "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils", "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "console", "resource://gre/modules/devtools/Console.jsm");

let loader = Cu.import("resource://gre/modules/commonjs/toolkit/loader.js", {}).Loader;
let Promise = Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js", {}).Promise;

this.EXPORTED_SYMBOLS = ["devtools"];





let loaderGlobals = {
  console: console,
  _Iterator: Iterator,
  loader: {
    lazyGetter: XPCOMUtils.defineLazyGetter.bind(XPCOMUtils),
    lazyImporter: XPCOMUtils.defineLazyModuleGetter.bind(XPCOMUtils)
  }
}


var BuiltinProvider = {
  load: function(done) {
    this.loader = new loader.Loader({
      modules: {
        "toolkit/loader": loader
      },
      paths: {
        "": "resource://gre/modules/commonjs/",
        "main": "resource:///modules/devtools/main.js",
        "devtools": "resource:///modules/devtools",
        "devtools/server": "resource://gre/modules/devtools/server",

        
        "xpcshell-test": "resource://test"
      },
      globals: loaderGlobals
    });

    return Promise.resolve(undefined);
  },

  unload: function(reason) {
    loader.unload(this.loader, reason);
    delete this.loader;
  },
};




var SrcdirProvider = {
  fileURI: function(path) {
    let file = new FileUtils.File(path);
    return Services.io.newFileURI(file).spec;
  },

  load: function(done) {
    let srcdir = Services.prefs.getComplexValue("devtools.loader.srcdir",
                                                Ci.nsISupportsString);
    srcdir = OS.Path.normalize(srcdir.data.trim());
    let devtoolsDir = OS.Path.join(srcdir, "browser", "devtools");
    let devtoolsURI = this.fileURI(devtoolsDir);
    let toolkitURI = this.fileURI(OS.Path.join(srcdir, "toolkit", "devtools"));
    let serverURI = this.fileURI(OS.Path.join(srcdir, "toolkit", "devtools", "server"));
    let mainURI = this.fileURI(OS.Path.join(srcdir, "browser", "devtools", "main.js"));
    this.loader = new loader.Loader({
      modules: {
        "toolkit/loader": loader
      },
      paths: {
        "": "resource://gre/modules/commonjs/",
        "devtools/server": serverURI,
        "devtools": devtoolsURI,
        "main": mainURI
      },
      globals: loaderGlobals
    });

    return this._writeManifest(devtoolsDir).then(null, Cu.reportError);
  },

  unload: function(reason) {
    loader.unload(this.loader, reason);
    delete this.loader;
  },

  _readFile: function(filename) {
    let deferred = Promise.defer();
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
    let deferred = Promise.defer();
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






this.devtools = {
  _provider: null,

  







  loadURI: function(id, uri) {
    let module = loader.Module(id, uri);
    return loader.load(this._provider.loader, module).exports;
  },

  








  main: function(id) {
    this._mainid = id;
    this._main = loader.main(this._provider.loader, id);

    
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
    this._provider.load();
    this.require = loader.Require(this._provider.loader, { id: "devtools" });

    if (this._mainid) {
      this.main(this._mainid);
    }
  },

  


  _chooseProvider: function() {
    if (Services.prefs.prefHasUserValue("devtools.loader.srcdir")) {
      this.setProvider(SrcdirProvider);
    } else {
      this.setProvider(BuiltinProvider);
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
};


devtools._chooseProvider();
