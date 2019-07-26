


"use strict";
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");



Services.prefs.setBoolPref("devtools.debugger.log", true);

Services.prefs.setBoolPref("devtools.debugger.remote-enabled", true);

Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");


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

    do_throw("head_dbg.js got console message: " + string + "\n");
  }
};

let consoleService = Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService);
consoleService.registerListener(listener);

function check_except(func)
{
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
  Cu.evalInSandbox("this.__name = '" + aName + "'", sandbox);
  return sandbox;
}

function addTestGlobal(aName)
{
  let global = testGlobal(aName);
  DebuggerServer.addTestGlobal(global);
  return global;
}

function getTestGlobalContext(aClient, aName, aCallback) {
  aClient.request({ "to": "root", "type": "listContexts" }, function(aResponse) {
    for each (let context in aResponse.contexts) {
      if (context.global == aName) {
        aCallback(context);
        return false;
      }
    }
    aCallback(null);
  });
}

function attachTestGlobalClient(aClient, aName, aCallback) {
  getTestGlobalContext(aClient, aName, function(aContext) {
    aClient.attachThread(aContext.actor, aCallback, { useSourceMaps: true });
  });
}

function attachTestGlobalClientAndResume(aClient, aName, aCallback) {
  attachTestGlobalClient(aClient, aName, function(aResponse, aThreadClient) {
    aThreadClient.resume(function(aResponse) {
      aCallback(aResponse, aThreadClient);
    });
  })
}

function getTestTab(aClient, aName, aCallback) {
  gClient.listTabs(function (aResponse) {
    for (let tab of aResponse.tabs) {
      if (tab.title === aName) {
        aCallback(tab);
        return;
      }
    }
    aCallback(null);
  });
}

function attachTestTab(aClient, aName, aCallback) {
  getTestTab(aClient, aName, function (aTab) {
    gClient.attachTab(aTab.actor, aCallback);
  });
}

function attachTestTabAndResume(aClient, aName, aCallback) {
  attachTestTab(aClient, aName, function (aResponse, aTabClient) {
    aClient.attachThread(aResponse.threadActor, function (aResponse, aThreadClient) {
      aThreadClient.resume(function (aResponse) {
        aCallback(aResponse, aTabClient, aThreadClient);
      });
    }, { useSourceMaps: true });
  });
}




function initTestDebuggerServer()
{
  DebuggerServer.addActors("resource://test/testactors.js");
  
  DebuggerServer.init(function () { return true; });
}

function initSourcesBackwardsCompatDebuggerServer()
{
  DebuggerServer.addActors("chrome://global/content/devtools/dbg-browser-actors.js");
  DebuggerServer.addActors("resource://test/testcompatactors.js");
  DebuggerServer.init(function () { return true; });
}

function finishClient(aClient)
{
  aClient.close(function() {
    do_test_finished();
  });
}





function getFilePath(aName)
{
  let file = do_get_file(aName);
  let path = Services.io.newFileURI(file).spec;
  let filePrePath = "file://";
  if ("nsILocalFileWin" in Ci &&
      file instanceof Ci.nsILocalFileWin) {
    filePrePath += "/";
  }
  return path.slice(filePrePath.length);
}
