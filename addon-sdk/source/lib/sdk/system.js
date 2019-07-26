



'use strict';

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci, CC } = require('chrome');
const options = require('@loader/options');
const file = require('./io/file');
const runtime = require("./system/runtime");
var cfxArgs = require("@test/options");

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

  
  if ('resultFile' in options && options.resultFile) {
    let mode = PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE;
    let stream = openFile(options.resultFile, mode);
    let status = code ? 'FAIL' : 'OK';
    stream.write(status, status.length);
    stream.flush();
    stream.close();
    if (cfxArgs.parseable) {
      console.log('wrote to resultFile');
    }
  }

  forcedExit = true;
  appStartup.quit(E_FORCE);
};



let stdout = Object.freeze({ write: dump, end: dump });
exports.stdout = stdout;
exports.stderr = stdout;
















exports.pathFor = function pathFor(id) {
  return directoryService.get(id, Ci.nsIFile).path;
};






exports.platform = runtime.OS.toLowerCase();





exports.architecture = runtime.XPCOMABI.split('_')[0];





exports.compiler = runtime.XPCOMABI.split('_')[1];




exports.build = appInfo.appBuildID;







exports.id = appInfo.ID;




exports.name = appInfo.name;




exports.version = appInfo.version;




exports.platformVersion = appInfo.platformVersion;





exports.vendor = appInfo.vendor;
