



"use strict";

this.EXPORTED_SYMBOLS = [
  "makeFakeAppDir",
];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");


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

