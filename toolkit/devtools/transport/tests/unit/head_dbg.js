


"use strict";
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;
const CC = Components.Constructor;


Cu.import("resource://testing-common/AppInfo.jsm");
updateAppInfo();

const { devtools } =
  Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const { Promise: promise } =
  Cu.import("resource://gre/modules/Promise.jsm", {});

const Services = devtools.require("Services");
const DevToolsUtils = devtools.require("devtools/toolkit/DevToolsUtils.js");








Services.prefs.setBoolPref("devtools.debugger.remote-enabled", true);

function tryImport(url) {
  try {
    Cu.import(url);
  } catch (e) {
    dump("Error importing " + url + "\n");
    dump(DevToolsUtils.safeErrorString(e) + "\n");
    throw e;
  }
}

tryImport("resource://gre/modules/devtools/dbg-server.jsm");
tryImport("resource://gre/modules/devtools/dbg-client.jsm");
tryImport("resource://gre/modules/devtools/Loader.jsm");

function testExceptionHook(ex) {
  try {
    do_report_unexpected_exception(ex);
  } catch(ex) {
    return {throw: ex}
  }
  return undefined;
}


function scriptErrorFlagsToKind(aFlags) {
  var kind;
  if (aFlags & Ci.nsIScriptError.warningFlag)
    kind = "warning";
  if (aFlags & Ci.nsIScriptError.exceptionFlag)
    kind = "exception";
  else
    kind = "error";

  if (aFlags & Ci.nsIScriptError.strictFlag)
    kind = "strict " + kind;

  return kind;
}


function dbg_assert(cond, e) {
  if (!cond) {
    throw e;
  }
}



let errorCount = 0;
let listener = {
  observe: function (aMessage) {
    errorCount++;
    try {
      
      
      var scriptError = aMessage.QueryInterface(Ci.nsIScriptError);
      dump(aMessage.sourceName + ":" + aMessage.lineNumber + ": " +
           scriptErrorFlagsToKind(aMessage.flags) + ": " +
           aMessage.errorMessage + "\n");
      var string = aMessage.errorMessage;
    } catch (x) {
      
      
      try {
        var string = "" + aMessage.message;
      } catch (x) {
        var string = "<error converting error message to string>";
      }
    }

    
    while (DebuggerServer.xpcInspector.eventLoopNestLevel > 0) {
      DebuggerServer.xpcInspector.exitNestedEventLoop();
    }

    
    if (!(aMessage.flags & Ci.nsIScriptError.strictFlag)) {
      do_throw("head_dbg.js got console message: " + string + "\n");
    }
  }
};

let consoleService = Cc["@mozilla.org/consoleservice;1"]
                     .getService(Ci.nsIConsoleService);
consoleService.registerListener(listener);

function check_except(func) {
  try {
    func();
  } catch (e) {
    do_check_true(true);
    return;
  }
  dump("Should have thrown an exception: " + func.toString());
  do_check_true(false);
}

function testGlobal(aName) {
  let systemPrincipal = Cc["@mozilla.org/systemprincipal;1"]
    .createInstance(Ci.nsIPrincipal);

  let sandbox = Cu.Sandbox(systemPrincipal);
  sandbox.__name = aName;
  return sandbox;
}

function addTestGlobal(aName)
{
  let global = testGlobal(aName);
  DebuggerServer.addTestGlobal(global);
  return global;
}



function getTestTab(aClient, aTitle, aCallback) {
  aClient.listTabs(function (aResponse) {
    for (let tab of aResponse.tabs) {
      if (tab.title === aTitle) {
        aCallback(tab);
        return;
      }
    }
    aCallback(null);
  });
}



function attachTestTab(aClient, aTitle, aCallback) {
  getTestTab(aClient, aTitle, function (aTab) {
    aClient.attachTab(aTab.actor, aCallback);
  });
}





function attachTestThread(aClient, aTitle, aCallback) {
  attachTestTab(aClient, aTitle, function (aResponse, aTabClient) {
    function onAttach(aResponse, aThreadClient) {
      aCallback(aResponse, aTabClient, aThreadClient);
    }
    aTabClient.attachThread({ useSourceMaps: true }, onAttach);
  });
}





function attachTestTabAndResume(aClient, aTitle, aCallback) {
  attachTestThread(aClient, aTitle, function(aResponse, aTabClient, aThreadClient) {
    aThreadClient.resume(function (aResponse) {
      aCallback(aResponse, aTabClient, aThreadClient);
    });
  });
}




function initTestDebuggerServer() {
  DebuggerServer.registerModule("devtools/server/actors/script");
  DebuggerServer.registerModule("xpcshell-test/testactors");
  
  DebuggerServer.init(function () { return true; });
}

function finishClient(aClient) {
  aClient.close(function() {
    do_test_finished();
  });
}




function getFileUrl(aName, aAllowMissing=false) {
  let file = do_get_file(aName, aAllowMissing);
  return Services.io.newFileURI(file).spec;
}





function getFilePath(aName, aAllowMissing=false) {
  let file = do_get_file(aName, aAllowMissing);
  let path = Services.io.newFileURI(file).spec;
  let filePrePath = "file://";
  if ("nsILocalFileWin" in Ci &&
      file instanceof Ci.nsILocalFileWin) {
    filePrePath += "/";
  }
  return path.slice(filePrePath.length);
}

Cu.import("resource://gre/modules/NetUtil.jsm");





function getTestTempFile(fileName, allowMissing) {
  let thisTest = _TEST_FILE.toString().replace(/\\/g, "/");
  thisTest = thisTest.substring(thisTest.lastIndexOf("/") + 1);
  thisTest = thisTest.replace(/\..*$/, "");
  return do_get_file(fileName + "-" + thisTest, allowMissing);
}

function writeTestTempFile(aFileName, aContent) {
  let file = getTestTempFile(aFileName, true);
  let stream = Cc["@mozilla.org/network/file-output-stream;1"]
    .createInstance(Ci.nsIFileOutputStream);
  stream.init(file, -1, -1, 0);
  try {
    do {
      let numWritten = stream.write(aContent, aContent.length);
      aContent = aContent.slice(numWritten);
    } while (aContent.length > 0);
  } finally {
    stream.close();
  }
}



function socket_transport() {
  if (!DebuggerServer.listeningSockets) {
    DebuggerServer.openListener(-1);
  }
  let port = DebuggerServer._listeners[0].port;
  do_print("Debugger server port is " + port);
  return debuggerSocketConnect("127.0.0.1", port);
}

function local_transport() {
  return DebuggerServer.connectPipe();
}



let gReallyLong;
function really_long() {
  if (gReallyLong) {
    return gReallyLong;
  }
  let ret = "0123456789";
  for (let i = 0; i < 18; i++) {
    ret += ret;
  }
  gReallyLong = ret;
  return ret;
}
