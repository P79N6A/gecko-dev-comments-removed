



"use strict";

const { Cc, Ci, Cu } = require("chrome");

const { Promise: promise } = require("resource://gre/modules/Promise.jsm");

const { OS } = Cu.import("resource://gre/modules/osfile.jsm", {});
const { TextEncoder, TextDecoder } = Cu.import('resource://gre/modules/commonjs/toolkit/loader.js', {});
const gcli = require("gcli/index");

loader.lazyGetter(this, "prefBranch", function() {
  let prefService = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefService);
  return prefService.getBranch(null).QueryInterface(Ci.nsIPrefBranch2);
});

loader.lazyGetter(this, "supportsString", function() {
  return Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
});

loader.lazyImporter(this, "NetUtil", "resource://gre/modules/NetUtil.jsm");

const PREF_DIR = "devtools.commands.dir";






function loadItemsFromMozDir() {
  let dirName = prefBranch.getComplexValue(PREF_DIR,
                                           Ci.nsISupportsString).data.trim();
  if (dirName == "") {
    return promise.resolve([]);
  }

  
  if (dirName.indexOf("~") == 0) {
    let dirService = Cc["@mozilla.org/file/directory_service;1"]
                      .getService(Ci.nsIProperties);
    let homeDirFile = dirService.get("Home", Ci.nsIFile);
    let homeDir = homeDirFile.path;
    dirName = dirName.substr(1);
    dirName = homeDir + dirName;
  }

  
  
  let statPromise = OS.File.stat(dirName);
  statPromise = statPromise.then(
    function onSuccess(stat) {
      if (!stat.isDir) {
        throw new Error("'" + dirName + "' is not a directory.");
      }
    },
    function onFailure(reason) {
      if (reason instanceof OS.File.Error && reason.becauseNoSuchFile) {
        throw new Error("'" + dirName + "' does not exist.");
      } else {
        throw reason;
      }
    }
  );

  
  
  return statPromise.then(() => {
    let itemPromises = [];

    let iterator = new OS.File.DirectoryIterator(dirName);
    let iterPromise = iterator.forEach(entry => {
      if (entry.name.match(/.*\.mozcmd$/) && !entry.isDir) {
        itemPromises.push(loadCommandFile(entry));
      }
    });

    return iterPromise.then(() => {
      iterator.close();
      return promise.all(itemPromises).then((itemsArray) => {
        return itemsArray.reduce((prev, curr) => {
          return prev.concat(curr);
        }, []);
      });
    }, reason => { iterator.close(); throw reason; });
  });
}

exports.mozDirLoader = function(name) {
  return loadItemsFromMozDir().then(items => {
    return { items: items };
  });
};






function loadCommandFile(entry) {
  let readPromise = OS.File.read(entry.path);
  return readPromise = readPromise.then(array => {
    let decoder = new TextDecoder();
    let source = decoder.decode(array);
    var principal = Cc["@mozilla.org/systemprincipal;1"]
                      .createInstance(Ci.nsIPrincipal);

    let sandbox = new Cu.Sandbox(principal, {
      sandboxName: entry.path
    });
    let data = Cu.evalInSandbox(source, sandbox, "1.8", entry.name, 1);

    if (!Array.isArray(data)) {
      console.error("Command file '" + entry.name + "' does not have top level array.");
      return;
    }

    return data;
  });
}

exports.items = [
  {
    name: "cmd",
    get hidden() {
      return !prefBranch.prefHasUserValue(PREF_DIR);
    },
    description: gcli.lookup("cmdDesc")
  },
  {
    name: "cmd refresh",
    description: gcli.lookup("cmdRefreshDesc"),
    get hidden() {
      return !prefBranch.prefHasUserValue(PREF_DIR);
    },
    exec: function(args, context) {
      gcli.load();

      let dirName = prefBranch.getComplexValue(PREF_DIR,
                                              Ci.nsISupportsString).data.trim();
      return gcli.lookupFormat("cmdStatus2", [ dirName ]);
    }
  },
  {
    name: "cmd setdir",
    description: gcli.lookup("cmdSetdirDesc"),
    manual: gcli.lookup("cmdSetdirManual2"),
    params: [
      {
        name: "directory",
        description: gcli.lookup("cmdSetdirDirectoryDesc"),
        type: {
          name: "file",
          filetype: "directory",
          existing: "yes"
        },
        defaultValue: null
      }
    ],
    returnType: "string",
    get hidden() {
      return true; 
    },
    exec: function(args, context) {
      supportsString.data = args.directory;
      prefBranch.setComplexValue(PREF_DIR, Ci.nsISupportsString, supportsString);

      gcli.load();

      return gcli.lookupFormat("cmdStatus3", [ args.directory ]);
    }
  }
];
