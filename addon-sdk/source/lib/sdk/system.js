


'use strict';

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci, CC } = require('chrome');
const options = require('@loader/options');
const file = require('./io/file');
const runtime = require("./system/runtime");
const { when: unload } = require("./system/unload");

const appStartup = Cc['@mozilla.org/toolkit/app-startup;1'].
                   getService(Ci.nsIAppStartup);
const appInfo = Cc["@mozilla.org/xre/app-info;1"].
                getService(Ci.nsIXULAppInfo);
const directoryService = Cc['@mozilla.org/file/directory_service;1'].
                         getService(Ci.nsIProperties);

const PR_WRONLY = parseInt("0x02");
const PR_CREATE_FILE = parseInt("0x08");
const PR_APPEND = parseInt("0x10");
const PR_TRUNCATE = parseInt("0x20");

function openFile(path, mode) {
  let file = Cc["@mozilla.org/file/local;1"].
             createInstance(Ci.nsILocalFile);
  file.initWithPath(path);
  let stream = Cc["@mozilla.org/network/file-output-stream;1"].
               createInstance(Ci.nsIFileOutputStream);
  stream.init(file, mode, -1, 0);
  return stream
}

const { eAttemptQuit: E_ATTEMPT, eForceQuit: E_FORCE } = appStartup;




exports.staticArgs = options.staticArgs;





exports.env = require('./system/environment').env;






let forcedExit = false;
exports.exit = function exit(code) {
  if (forcedExit) {
    
    
    return;
  }

  let resultsFile = 'resultFile' in options && options.resultFile;
  function unloader() {
    if (!options.resultFile) {
      return;
    }

    
    let mode = PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE;
    let stream = openFile(options.resultFile, mode);
    let status = code ? 'FAIL' : 'OK';
    stream.write(status, status.length);
    stream.flush();
    stream.close();
    return;
  }

  if (code == 0) {
    forcedExit = true;
  }

  
  if (options.noQuit) {
    return unload(unloader);
  }

  unloader();
  appStartup.quit(code ? E_ATTEMPT : E_FORCE);
};



let stdout = Object.freeze({ write: dump, end: dump });
exports.stdout = stdout;
exports.stderr = stdout;
















exports.pathFor = function pathFor(id) {
  return directoryService.get(id, Ci.nsIFile).path;
};






exports.platform = runtime.OS.toLowerCase();

const [, architecture, compiler] = runtime.XPCOMABI ?
                                   runtime.XPCOMABI.match(/^([^-]*)-(.*)$/) :
                                   [, null, null];





exports.architecture = architecture;





exports.compiler = compiler;




exports.build = appInfo.appBuildID;







exports.id = appInfo.ID;




exports.name = appInfo.name;




exports.version = appInfo.version;




exports.platformVersion = appInfo.platformVersion;





exports.vendor = appInfo.vendor;
