



var EXPORTED_SYMBOLS = ['Collector','Runner','events', 'runTestFile', 'log',
                        'timers', 'persisted', 'shutdownApplication'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const TIMEOUT_SHUTDOWN_HTTPD = 15000;

Cu.import("resource://gre/modules/Services.jsm");

Cu.import('resource://mozmill/stdlib/httpd.js');

var broker = {};  Cu.import('resource://mozmill/driver/msgbroker.js', broker);
var assertions = {}; Cu.import('resource://mozmill/modules/assertions.js', assertions);
var errors = {}; Cu.import('resource://mozmill/modules/errors.js', errors);
var os = {};      Cu.import('resource://mozmill/stdlib/os.js', os);
var strings = {}; Cu.import('resource://mozmill/stdlib/strings.js', strings);
var arrays = {};  Cu.import('resource://mozmill/stdlib/arrays.js', arrays);
var withs = {};   Cu.import('resource://mozmill/stdlib/withs.js', withs);
var utils = {};   Cu.import('resource://mozmill/stdlib/utils.js', utils);

var securableModule = {};
Cu.import('resource://mozmill/stdlib/securable-module.js', securableModule);

var uuidgen = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);

var httpd = null;
var persisted = {};

var assert = new assertions.Assert();
var expect = new assertions.Expect();

var mozmill = undefined;
var mozelement = undefined;
var modules = undefined;

var timers = [];










function shutdownApplication(aFlags) {
  var flags = Ci.nsIAppStartup.eForceQuit;

  if (aFlags) {
    flags |= aFlags;
  }

  
  
  
  
  let cancelQuit = Components.classes["@mozilla.org/supports-PRBool;1"].
                   createInstance(Components.interfaces.nsISupportsPRBool);
  Services.obs.notifyObservers(cancelQuit, "quit-application-requested", null);

  
  
  var event = {
    notify: function(timer) {
      Services.startup.quit(flags);
    }
  }

  var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(event, 100, Ci.nsITimer.TYPE_ONE_SHOT);
}

function stateChangeBase(possibilties, restrictions, target, cmeta, v) {
  if (possibilties) {
    if (!arrays.inArray(possibilties, v)) {
      
      return;
    }
  }

  if (restrictions) {
    for (var i in restrictions) {
      var r = restrictions[i];
      if (!r(v)) {
        
        return;
      }
    }
  }

  
  events[target] = v;
  events.fireEvent(cmeta, target);
}


var events = {
  appQuit           : false,
  currentModule     : null,
  currentState      : null,
  currentTest       : null,
  shutdownRequested : false,
  userShutdown      : null,
  userShutdownTimer : null,

  listeners       : {},
  globalListeners : []
}

events.setState = function (v) {
  return stateChangeBase(['dependencies', 'setupModule', 'teardownModule',
                          'test', 'setupTest', 'teardownTest', 'collection'],
                          null, 'currentState', 'setState', v);
}

events.toggleUserShutdown = function (obj){
  if (!this.userShutdown) {
    this.userShutdown = obj;

    var event = {
      notify: function(timer) {
       events.toggleUserShutdown(obj);
      }
    }

    this.userShutdownTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this.userShutdownTimer.initWithCallback(event, obj.timeout, Ci.nsITimer.TYPE_ONE_SHOT);

  } else {
    this.userShutdownTimer.cancel();

    
    
    if (!events.appQuit) {
      this.fail({'function':'events.toggleUserShutdown',
                 'message':'Shutdown expected but none detected before timeout',
                 'userShutdown': obj});

      var flags = Ci.nsIAppStartup.eAttemptQuit;
      if (events.isRestartShutdown()) {
        flags |= Ci.nsIAppStartup.eRestart;
      }

      shutdownApplication(flags);
    }
  }
}

events.isUserShutdown = function () {
  return this.userShutdown ? this.userShutdown["user"] : false;
}

events.isRestartShutdown = function () {
  return this.userShutdown.restart;
}

events.startShutdown = function (obj) {
  events.fireEvent('shutdown', obj);

  if (obj["user"]) {
    events.toggleUserShutdown(obj);
  } else {
    shutdownApplication(obj.flags);
  }
}

events.setTest = function (test) {
  test.__start__ = Date.now();
  test.__passes__ = [];
  test.__fails__ = [];

  events.currentTest = test;

  var obj = {'filename': events.currentModule.__file__,
             'name': test.__name__}
  events.fireEvent('setTest', obj);
}

events.endTest = function (test) {
  
  if (test === undefined) {
    test = events.currentTest;
  }

  
  
  if (!test || test.status === 'done')
    return;

  
  test.__end__ = Date.now();
  test.status = 'done';

  var obj = {'filename': events.currentModule.__file__,
             'passed': test.__passes__.length,
             'failed': test.__fails__.length,
             'passes': test.__passes__,
             'fails' : test.__fails__,
             'name'  : test.__name__,
             'time_start': test.__start__,
             'time_end': test.__end__}

  if (test.skipped) {
    obj['skipped'] = true;
    obj.skipped_reason = test.skipped_reason;
  }

  if (test.meta) {
    obj.meta = test.meta;
  }

  
  if (withs.startsWith(test.__name__, "test") || test.__fails__.length > 0) {
    events.fireEvent('endTest', obj);
  }
}

events.setModule = function (aModule) {
  aModule.__start__ = Date.now();
  aModule.__status__ = 'running';

  var result = stateChangeBase(null,
                               [function (aModule) {return (aModule.__file__ != undefined)}],
                               'currentModule', 'setModule', aModule);

  return result;
}

events.endModule = function (aModule) {
  
  if (aModule.__status__ === 'done')
    return;

  aModule.__end__ = Date.now();
  aModule.__status__ = 'done';

  var obj = {
    'filename': aModule.__file__,
    'time_start': aModule.__start__,
    'time_end': aModule.__end__
  }

  events.fireEvent('endModule', obj);
}

events.pass = function (obj) {
  
  if (events.currentTest) {
    events.currentTest.__passes__.push(obj);
  }

  for each (var timer in timers) {
    timer.actions.push(
      {"currentTest": events.currentModule.__file__ + "::" + events.currentTest.__name__,
       "obj": obj,
       "result": "pass"}
    );
  }

  events.fireEvent('pass', obj);
}

events.fail = function (obj) {
  var error = obj.exception;

  if (error) {
    
    obj.exception = {
      name: error.name,
      message: error.message,
      lineNumber: error.lineNumber,
      fileName: error.fileName,
      stack: error.stack
    };
  }

  
  if (events.currentTest) {
    events.currentTest.__fails__.push(obj);
  }

  for each (var time in timers) {
    timer.actions.push(
      {"currentTest": events.currentModule.__file__ + "::" + events.currentTest.__name__,
       "obj": obj,
       "result": "fail"}
    );
  }

  events.fireEvent('fail', obj);
}

events.skip = function (reason) {
  
  events.currentTest.skipped = true;
  events.currentTest.skipped_reason = reason;

  for (var timer of timers) {
    timer.actions.push(
      {"currentTest": events.currentModule.__file__ + "::" + events.currentTest.__name__,
       "obj": reason,
       "result": "skip"}
    );
  }

  events.fireEvent('skip', reason);
}

events.fireEvent = function (name, obj) {
  if (events.appQuit) {
    
    return;
  }

  if (this.listeners[name]) {
    for (var i in this.listeners[name]) {
      this.listeners[name][i](obj);
    }
  }

  for each(var listener in this.globalListeners) {
    listener(name, obj);
  }
}

events.addListener = function (name, listener) {
  if (this.listeners[name]) {
    this.listeners[name].push(listener);
  } else if (name == '') {
    this.globalListeners.push(listener)
  } else {
    this.listeners[name] = [listener];
  }
}

events.removeListener = function (listener) {
  for (var listenerIndex in this.listeners) {
    var e = this.listeners[listenerIndex];

    for (var i in e){
      if (e[i] == listener) {
        this.listeners[listenerIndex] = arrays.remove(e, i);
      }
    }
  }

  for (var i in this.globalListeners) {
    if (this.globalListeners[i] == listener) {
      this.globalListeners = arrays.remove(this.globalListeners, i);
    }
  }
}

events.persist = function () {
  try {
    events.fireEvent('persist', persisted);
  } catch (e) {
    events.fireEvent('error', "persist serialization failed.")
  }
}

events.firePythonCallback = function (obj) {
  obj['test'] = events.currentModule.__file__;
  events.fireEvent('firePythonCallback', obj);
}

events.screenshot = function (obj) {
  
  for (var attr in events.currentModule) {
    if (events.currentModule[attr] == events.currentTest) {
      var testName = attr;
      break;
    }
  }

  obj['test_file'] = events.currentModule.__file__;
  obj['test_name'] = testName;
  events.fireEvent('screenshot', obj);
}

var log = function (obj) {
  events.fireEvent('log', obj);
}


broker.addObject({'endTest': events.endTest,
                  'fail': events.fail,
                  'firePythonCallback': events.firePythonCallback,
                  'log': log,
                  'pass': events.pass,
                  'persist': events.persist,
                  'screenshot': events.screenshot,
                  'shutdown': events.startShutdown,
                 });

try {
  Cu.import('resource://jsbridge/modules/Events.jsm');

  events.addListener('', function (name, obj) {
    Events.fireEvent('mozmill.' + name, obj);
  });
} catch (e) {
  Services.console.logStringMessage("Event module of JSBridge not available.");
}





function AppQuitObserver() {
  this.runner = null;

  Services.obs.addObserver(this, "quit-application-requested", false);
}

AppQuitObserver.prototype = {
  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "quit-application-requested":
        Services.obs.removeObserver(this, "quit-application-requested");

        
        
        
        events.pass({'message': 'AppQuitObserver: ' + JSON.stringify(aData),
                     'userShutdown': events.userShutdown});

        if (this.runner) {
          this.runner.end();
        }

        if (httpd) {
          httpd.stop();
        }

        events.appQuit = true;

        break;
    }
  }
}

var appQuitObserver = new AppQuitObserver();




function Collector() {
  this.test_modules_by_filename = {};
  this.testing = [];
}

Collector.prototype.addHttpResource = function (aDirectory, aPath) {
  var fp = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
  fp.initWithPath(os.abspath(aDirectory, this.current_file));

  return httpd.addHttpResource(fp, aPath);
}

Collector.prototype.initTestModule = function (filename, testname) {
  var test_module = this.loadFile(filename, this);
  var has_restarted = !(testname == null);
  test_module.__tests__ = [];

  for (var i in test_module) {
    if (typeof(test_module[i]) == "function") {
      test_module[i].__name__ = i;

      
      
      
      if (i == "setupModule" && !has_restarted) {
        test_module.__setupModule__ = test_module[i];
      } else if (i == "setupTest") {
        test_module.__setupTest__ = test_module[i];
      } else if (i == "teardownTest") {
        test_module.__teardownTest__ = test_module[i];
      } else if (i == "teardownModule") {
        test_module.__teardownModule__ = test_module[i];
      } else if (withs.startsWith(i, "test")) {
        if (testname && (i != testname)) {
          continue;
        }

        testname = null;
        test_module.__tests__.push(test_module[i]);
      }
    }
  }

  test_module.collector = this;
  test_module.status = 'loaded';

  this.test_modules_by_filename[filename] = test_module;

  return test_module;
}

Collector.prototype.loadFile = function (path, collector) {
  var moduleLoader = new securableModule.Loader({
    rootPaths: ["resource://mozmill/modules/"],
    defaultPrincipal: "system",
    globals : { Cc: Cc,
                Ci: Ci,
                Cu: Cu,
                Cr: Components.results}
  });

  
  var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
  file.initWithPath(path);
  var uri = Services.io.newFileURI(file).spec;

  this.loadTestResources();

  var systemPrincipal = Services.scriptSecurityManager.getSystemPrincipal();
  var module = new Components.utils.Sandbox(systemPrincipal);
  module.assert = assert;
  module.Cc = Cc;
  module.Ci = Ci;
  module.Cr = Components.results;
  module.Cu = Cu;
  module.collector = collector;
  module.driver = moduleLoader.require("driver");
  module.elementslib = mozelement;
  module.errors = errors;
  module.expect = expect;
  module.findElement = mozelement;
  module.log = log;
  module.mozmill = mozmill;
  module.persisted = persisted;

  module.require = function (mod) {
    var loader = new securableModule.Loader({
      rootPaths: [Services.io.newFileURI(file.parent).spec,
                  "resource://mozmill/modules/"],
      defaultPrincipal: "system",
      globals : { assert: assert,
                  expect: expect,
                  mozmill: mozmill,
                  elementslib: mozelement,      
                  findElement: mozelement,
                  persisted: persisted,
                  Cc: Cc,
                  Ci: Ci,
                  Cu: Cu,
                  log: log }
    });

    if (modules != undefined) {
      loader.modules = modules;
    }

    var retval = loader.require(mod);
    modules = loader.modules;

    return retval;
  }

  if (collector != undefined) {
    collector.current_file = file;
    collector.current_path = path;
  }

  try {
    Services.scriptloader.loadSubScript(uri, module, "UTF-8");
  } catch (e) {
    var obj = {
      'filename': path,
      'passed': 0,
      'failed': 1,
      'passes': [],
      'fails' : [{'exception' : {
                    message: e.message,
                    filename: e.filename,
                    lineNumber: e.lineNumber}}],
      'name'  :'<TOP_LEVEL>'
    };

    events.fail({'exception': e});
    events.fireEvent('endTest', obj);
  }

  module.__file__ = path;
  module.__uri__ = uri;

  return module;
}

Collector.prototype.loadTestResources = function () {
  
  if (mozmill === undefined) {
    mozmill = {};
    Cu.import("resource://mozmill/driver/mozmill.js", mozmill);
  }
  if (mozelement === undefined) {
    mozelement = {};
    Cu.import("resource://mozmill/driver/mozelement.js", mozelement);
  }
}





function Httpd(aPort) {
  this.http_port = aPort;

  while (true) {
    try {
      var srv = new HttpServer();
      srv.registerContentType("sjs", "sjs");
      srv.identity.setPrimary("http", "localhost", this.http_port);
      srv.start(this.http_port);

      this._httpd = srv;
      break;
    }
    catch (e) {
      
      this.http_port++;
    }
  }
}

Httpd.prototype.addHttpResource = function (aDir, aPath) {
  var path = aPath ? ("/" + aPath + "/") : "/";

  try {
    this._httpd.registerDirectory(path, aDir);
    return 'http://localhost:' + this.http_port + path;
  }
  catch (e) {
    throw Error("Failure to register directory: " + aDir.path);
  }
};

Httpd.prototype.stop = function () {
  if (!this._httpd) {
    return;
  }

  var shutdown = false;
  this._httpd.stop(function () { shutdown = true; });

  assert.waitFor(function () {
    return shutdown;
  }, "Local HTTP server has been stopped", TIMEOUT_SHUTDOWN_HTTPD);

  this._httpd = null;
};

function startHTTPd() {
  if (!httpd) {
    
    httpd = new Httpd(43336);
  }
}


function Runner() {
  this.collector = new Collector();
  this.ended = false;

  var m = {}; Cu.import('resource://mozmill/driver/mozmill.js', m);
  this.platform = m.platform;

  events.fireEvent('startRunner', true);
}

Runner.prototype.end = function () {
  if (!this.ended) {
    this.ended = true;

    appQuitObserver.runner = null;

    events.endTest();
    events.endModule(events.currentModule);
    events.fireEvent('endRunner', true);
    events.persist();
  }
};

Runner.prototype.runTestFile = function (filename, name) {
  var module = this.collector.initTestModule(filename, name);
  this.runTestModule(module);
};

Runner.prototype.runTestModule = function (module) {
  appQuitObserver.runner = this;
  events.setModule(module);

  
  if (this.execFunction(module.__setupModule__, module)) {
    for (var test of module.__tests__) {
      if (events.shutdownRequested) {
        break;
      }

      
      if (this.execFunction(module.__setupTest__, module)) {
        this.execFunction(test);
      } else {
        this.skipFunction(test, module.__setupTest__.__name__ + " failed");
      }

      this.execFunction(module.__teardownTest__, module);
    }

  } else {
    for (var test of module.__tests__) {
      this.skipFunction(test, module.__setupModule__.__name__ + " failed");
    }
  }

  this.execFunction(module.__teardownModule__, module);
  events.endModule(module);
};

Runner.prototype.execFunction = function (func, arg) {
  if (typeof func !== "function" || events.shutdownRequested) {
    return true;
  }

  var isTest = withs.startsWith(func.__name__, "test");

  events.setState(isTest ? "test" : func.__name);
  events.setTest(func);

  
  if (func.EXCLUDED_PLATFORMS != undefined) {
    if (arrays.inArray(func.EXCLUDED_PLATFORMS, this.platform)) {
      events.skip("Platform exclusion");
      events.endTest(func);
      return false;
    }
  }

  
  if (func.__force_skip__ != undefined) {
    events.skip(func.__force_skip__);
    events.endTest(func);
    return false;
  }

  
  try {
    func(arg);
  } catch (e) {
    if (e instanceof errors.ApplicationQuitError) {
      events.shutdownRequested = true;
    } else {
      events.fail({'exception': e, 'test': func})
    }
  }

  
  
  
  if (events.isUserShutdown()) {
    events.shutdownRequested = true;
    events.toggleUserShutdown(events.userShutdown);
  }

  events.endTest(func);
  return events.currentTest.__fails__.length == 0;
};

function runTestFile(filename, name) {
  var runner = new Runner();
  runner.runTestFile(filename, name);
  runner.end();

  return true;
}

Runner.prototype.skipFunction = function (func, message) {
  events.setTest(func);
  events.skip(message);
  events.endTest(func);
};
