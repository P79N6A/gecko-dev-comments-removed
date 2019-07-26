



"use strict";

this.EXPORTED_SYMBOLS = [
  "getAppInfo",
  "updateAppInfo",
  "makeFakeAppDir",
  "createFakeCrash",
  "InspectedHealthReporter",
];


const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/services-common/utils.js");
Cu.import("resource://gre/modules/HealthReport.jsm");


let APP_INFO = {
  vendor: "Mozilla",
  name: "xpcshell",
  ID: "xpcshell@tests.mozilla.org",
  version: "1",
  appBuildID: "20121107",
  platformVersion: "p-ver",
  platformBuildID: "20121106",
  inSafeMode: false,
  logConsoleErrors: true,
  OS: "XPCShell",
  XPCOMABI: "noarch-spidermonkey",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIXULAppInfo, Ci.nsIXULRuntime]),
  invalidateCachesOnRestart: function() {},
};





this.getAppInfo = function () { return APP_INFO; }










this.updateAppInfo = function (obj) {
  obj = obj || APP_INFO;
  APP_INFO = obj;

  let id = Components.ID("{fbfae60b-64a4-44ef-a911-08ceb70b9f31}");
  let cid = "@mozilla.org/xre/app-info;1";
  let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);

  
  try {
    let existing = Components.manager.getClassObjectByContractID(cid, Ci.nsIFactory);
    registrar.unregisterFactory(id, existing);
  } catch (ex) {}

  let factory = {
    createInstance: function (outer, iid) {
      if (outer != null) {
        throw Cr.NS_ERROR_NO_AGGREGATION;
      }

      return obj.QueryInterface(iid);
    },
  };

  registrar.registerFactory(id, "XULAppInfo", cid, factory);
};


let gFakeAppDirectoryProvider;


















this.makeFakeAppDir = function () {
  let dirMode = OS.Constants.libc.S_IRWXU;
  let dirService = Cc["@mozilla.org/file/directory_service;1"]
                     .getService(Ci.nsIProperties);
  let baseFile = dirService.get("ProfD", Ci.nsIFile);
  let appD = baseFile.clone();
  appD.append("UAppData");

  if (gFakeAppDirectoryProvider) {
    return Promise.resolve(appD.path);
  }

  function makeDir(f) {
    if (f.exists()) {
      return;
    }

    dump("Creating directory: " + f.path + "\n");
    f.create(Ci.nsIFile.DIRECTORY_TYPE, dirMode);
  }

  makeDir(appD);

  let reportsD = appD.clone();
  reportsD.append("Crash Reports");

  let pendingD = reportsD.clone();
  pendingD.append("pending");
  let submittedD = reportsD.clone();
  submittedD.append("submitted");

  makeDir(reportsD);
  makeDir(pendingD);
  makeDir(submittedD);

  let provider = {
    getFile: function (prop, persistent) {
      persistent.value = true;
      if (prop == "UAppData") {
        return appD.clone();
      }

      throw Cr.NS_ERROR_FAILURE;
    },

    QueryInterace: function (iid) {
      if (iid.equals(Ci.nsIDirectoryServiceProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }

      throw Cr.NS_ERROR_NO_INTERFACE;
    },
  };

  
  dirService.QueryInterface(Ci.nsIDirectoryService)
            .registerProvider(provider);

  
  try {
    dirService.undefine("UAppData");
  } catch (ex) {};

  gFakeAppDirectoryProvider = provider;

  dump("Successfully installed fake UAppDir\n");
  return Promise.resolve(appD.path);
};












this.createFakeCrash = function (submitted=false, date=new Date()) {
  let id = CommonUtils.generateUUID();
  let filename;

  let paths = ["Crash Reports"];
  let mode;

  if (submitted) {
    paths.push("submitted");
    filename = "bp-" + id + ".txt";
    mode = OS.Constants.libc.S_IRUSR | OS.Constants.libc.S_IWUSR |
           OS.Constants.libc.S_IRGRP | OS.Constants.libc.S_IROTH;
  } else {
    paths.push("pending");
    filename = id + ".dmp";
    mode = OS.Constants.libc.S_IRUSR | OS.Constants.libc.S_IWUSR;
  }

  paths.push(filename);

  let file = FileUtils.getFile("UAppData", paths, true);
  file.create(file.NORMAL_FILE_TYPE, mode);
  file.lastModifiedTime = date.getTime();
  dump("Created fake crash: " + id + "\n");

  return id;
};







this.InspectedHealthReporter = function (branch, policy) {
  HealthReporter.call(this, branch, policy);

  this.onStorageCreated = null;
  this.onCollectorInitialized = null;
  this.collectorShutdownCount = 0;
  this.storageCloseCount = 0;
}

InspectedHealthReporter.prototype = {
  __proto__: HealthReporter.prototype,

  _onStorageCreated: function (storage) {
    if (this.onStorageCreated) {
      this.onStorageCreated(storage);
    }

    return HealthReporter.prototype._onStorageCreated.call(this, storage);
  },

  _initializeCollector: function () {
    for (let result of HealthReporter.prototype._initializeCollector.call(this)) {
      yield result;
    }

    if (this.onInitializeCollectorFinished) {
      this.onInitializeCollectorFinished();
    }
  },

  _onCollectorInitialized: function () {
    if (this.onCollectorInitialized) {
      this.onCollectorInitialized();
    }

    return HealthReporter.prototype._onCollectorInitialized.call(this);
  },

  _onCollectorShutdown: function () {
    this.collectorShutdownCount++;

    return HealthReporter.prototype._onCollectorShutdown.call(this);
  },

  _onStorageClose: function () {
    this.storageCloseCount++;

    return HealthReporter.prototype._onStorageClose.call(this);
  },
};

