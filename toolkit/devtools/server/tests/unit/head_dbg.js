


"use strict";
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const { worker } = Cu.import("resource://gre/modules/devtools/worker-loader.js", {})
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
const { promiseInvoke } = devtools.require("devtools/async-utils");

const Services = devtools.require("Services");


Services.prefs.setBoolPref("devtools.debugger.log", true);

Services.prefs.setBoolPref("devtools.debugger.remote-enabled", true);

const DevToolsUtils = devtools.require("devtools/toolkit/DevToolsUtils.js");
const { DebuggerServer } = devtools.require("devtools/server/main");
const { DebuggerServer: WorkerDebuggerServer } = worker.require("devtools/server/main");

function dumpn(msg) {
  dump("DBG-TEST: " + msg + "\n");
}

function tryImport(url) {
  try {
    Cu.import(url);
  } catch (e) {
    dumpn("Error importing " + url);
    dumpn(DevToolsUtils.safeErrorString(e));
    throw e;
  }
}

tryImport("resource://gre/modules/devtools/dbg-client.jsm");
tryImport("resource://gre/modules/devtools/Loader.jsm");
tryImport("resource://gre/modules/devtools/Console.jsm");

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
      dumpn(aMessage.sourceName + ":" + aMessage.lineNumber + ": " +
            scriptErrorFlagsToKind(aMessage.flags) + ": " +
            aMessage.errorMessage);
      var string = aMessage.errorMessage;
    } catch (x) {
      
      
      try {
        var string = "" + aMessage.message;
      } catch (x) {
        var string = "<error converting error message to string>";
      }
    }

    
    while (DebuggerServer.xpcInspector
           && DebuggerServer.xpcInspector.eventLoopNestLevel > 0) {
      DebuggerServer.xpcInspector.exitNestedEventLoop();
    }

    
    
    
    
    
    
    
    
    
    dumpn("head_dbg.js observed a console message: " + string);
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
  dumpn("Should have thrown an exception: " + func.toString());
  do_check_true(false);
}

function testGlobal(aName) {
  let systemPrincipal = Cc["@mozilla.org/systemprincipal;1"]
    .createInstance(Ci.nsIPrincipal);

  let sandbox = Cu.Sandbox(systemPrincipal);
  sandbox.__name = aName;
  return sandbox;
}

function addTestGlobal(aName, aServer = DebuggerServer)
{
  let global = testGlobal(aName);
  aServer.addTestGlobal(global);
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
  attachTestTab(aClient, aTitle, function (aTabResponse, aTabClient) {
    function onAttach(aResponse, aThreadClient) {
      aCallback(aResponse, aTabClient, aThreadClient, aTabResponse);
    }
    aTabClient.attachThread({
      useSourceMaps: true,
      autoBlackBox: true
    }, onAttach);
  });
}





function attachTestTabAndResume(aClient, aTitle, aCallback) {
  attachTestThread(aClient, aTitle, function(aResponse, aTabClient, aThreadClient) {
    aThreadClient.resume(function (aResponse) {
      aCallback(aResponse, aTabClient, aThreadClient);
    });
  });
}




function initTestDebuggerServer(aServer = DebuggerServer)
{
  aServer.registerModule("devtools/server/actors/script");
  aServer.registerModule("xpcshell-test/testactors");
  
  aServer.init(function () { return true; });
}

function initTestTracerServer(aServer = DebuggerServer)
{
  aServer.registerModule("devtools/server/actors/script");
  aServer.registerModule("xpcshell-test/testactors");
  aServer.registerModule("devtools/server/actors/tracer");
  
  aServer.init(function () { return true; });
}

function finishClient(aClient)
{
  aClient.close(function() {
    do_test_finished();
  });
}




function getFileUrl(aName, aAllowMissing=false) {
  let file = do_get_file(aName, aAllowMissing);
  return Services.io.newFileURI(file).spec;
}





function getFilePath(aName, aAllowMissing=false)
{
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




function readFile(aFileName) {
  let f = do_get_file(aFileName);
  let s = Cc["@mozilla.org/network/file-input-stream;1"]
    .createInstance(Ci.nsIFileInputStream);
  s.init(f, -1, -1, false);
  try {
    return NetUtil.readInputStreamToString(s, s.available());
  } finally {
    s.close();
  }
}

function writeFile(aFileName, aContent) {
  let file = do_get_file(aFileName, true);
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

function connectPipeTracing() {
  return new TracingTransport(DebuggerServer.connectPipe());
}

function TracingTransport(childTransport) {
  this.hooks = null;
  this.child = childTransport;
  this.child.hooks = this;

  this.expectations = [];
  this.packets = [];
  this.checkIndex = 0;
}

TracingTransport.prototype = {
  
  normalize: function(packet) {
    return JSON.parse(JSON.stringify(packet, (key, value) => {
      if (key === "to" || key === "from" || key === "actor") {
        return "<actorid>";
      }
      return value;
    }));
  },
  send: function(packet) {
    this.packets.push({
      type: "sent",
      packet: this.normalize(packet)
    });
    return this.child.send(packet);
  },
  close: function() {
    return this.child.close();
  },
  ready: function() {
    return this.child.ready();
  },
  onPacket: function(packet) {
    this.packets.push({
      type: "received",
      packet: this.normalize(packet)
    });
    this.hooks.onPacket(packet);
  },
  onClosed: function() {
    this.hooks.onClosed();
  },

  expectSend: function(expected) {
    let packet = this.packets[this.checkIndex++];
    do_check_eq(packet.type, "sent");
    deepEqual(packet.packet, this.normalize(expected));
  },

  expectReceive: function(expected) {
    let packet = this.packets[this.checkIndex++];
    do_check_eq(packet.type, "received");
    deepEqual(packet.packet, this.normalize(expected));
  },

  
  
  dumpLog: function() {
    for (let entry of this.packets) {
      if (entry.type === "sent") {
        dumpn("trace.expectSend(" + entry.packet + ");");
      } else {
        dumpn("trace.expectReceive(" + entry.packet + ");");
      }
    }
  }
};

function StubTransport() { }
StubTransport.prototype.ready = function () {};
StubTransport.prototype.send  = function () {};
StubTransport.prototype.close = function () {};

function executeSoon(aFunc) {
  Services.tm.mainThread.dispatch({
    run: DevToolsUtils.makeInfallible(aFunc)
  }, Ci.nsIThread.DISPATCH_NORMAL);
}









let do_check_true_old = do_check_true;
let do_check_true = function (condition) {
  do_check_true_old(condition);
};

let do_check_false_old = do_check_false;
let do_check_false = function (condition) {
  do_check_false_old(condition);
};

let do_check_eq_old = do_check_eq;
let do_check_eq = function (left, right) {
  do_check_eq_old(left, right);
};

let do_check_neq_old = do_check_neq;
let do_check_neq = function (left, right) {
  do_check_neq_old(left, right);
};

let do_check_matches_old = do_check_matches;
let do_check_matches = function (pattern, value) {
  do_check_matches_old(pattern, value);
};




const Async = target => new Proxy(target, Async);
Async.get = (target, name) =>
  typeof(target[name]) === "function" ? asyncall.bind(null, target[name], target) :
  target[name];



const asyncall = (fn, self, ...args) =>
  new Promise((...etc) => fn.call(self, ...args, ...etc));

const Test = task => () => {
  add_task(task);
  run_next_test();
};

const assert = do_check_true;








function waitForEvent(client, event) {
  dumpn("Waiting for event: " + event);
  return new Promise((resolve, reject) => {
    client.addOneTimeListener(event, (_, packet) => resolve(packet));
  });
}







function waitForPause(client) {
  return waitForEvent(client, "paused");
}

















function executeOnNextTickAndWaitForPause(action, client) {
  const paused = waitForPause(client);
  executeSoon(action);
  return paused;
}














function rdpRequest(client, method, ...args) {
  return promiseInvoke(client, method, ...args)
    .then(response => {
      const { error, message } = response;
      if (error) {
        throw new Error(error + ": " + message);
      }
      return response;
    });
}








function setBreakpoint(threadClient, breakpointOptions) {
  dumpn("Setting a breakpoint: " + JSON.stringify(breakpointOptions, null, 2));
  return rdpRequest(threadClient, threadClient.setBreakpoint, breakpointOptions);
}







function resume(threadClient) {
  dumpn("Resuming.");
  return rdpRequest(threadClient, threadClient.resume);
}







function interrupt(threadClient) {
  dumpn("Interrupting.");
  return rdpRequest(threadClient, threadClient.interrupt);
}









function resumeAndWaitForPause(client, threadClient) {
  const paused = waitForPause(client);
  return resume(threadClient).then(() => paused);
}







function getSources(threadClient) {
  dumpn("Getting sources.");
  return rdpRequest(threadClient, threadClient.getSources);
}









function stepIn(client, threadClient) {
  dumpn("Stepping in.");
  const paused = waitForPause(client);
  return rdpRequest(threadClient, threadClient.stepIn)
    .then(() => paused);
}










function getFrames(threadClient, first, count) {
  dumpn("Getting frames.");
  return rdpRequest(threadClient, threadClient.getFrames, first, count);
}







function blackBox(sourceClient) {
  dumpn("Black boxing source: " + sourceClient.actor);
  return rdpRequest(sourceClient, sourceClient.blackBox);
}







function unBlackBox(sourceClient) {
  dumpn("Un-black boxing source: " + sourceClient.actor);
  return rdpRequest(sourceClient, sourceClient.unblackBox);
}
