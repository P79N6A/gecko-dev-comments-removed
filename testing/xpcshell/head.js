













































var _quit = false;
var _passed = true;
var _tests_pending = 0;
var _passedChecks = 0, _falsePassedChecks = 0;
var _cleanupFunctions = [];
var _pendingTimers = [];

function _dump(str) {
  if (typeof _XPCSHELL_PROCESS == "undefined") {
    dump(str);
  } else {
    dump(_XPCSHELL_PROCESS + ": " + str);
  }
}



let (ios = Components.classes["@mozilla.org/network/io-service;1"]
           .getService(Components.interfaces.nsIIOService2)) {
  ios.manageOfflineStatus = false;
  ios.offline = false;
}






try { 
  let processType = Components.classes["@mozilla.org/xre/runtime;1"].
    getService(Components.interfaces.nsIXULRuntime).processType;
  if (processType == Components.interfaces.nsIXULRuntime.PROCESS_TYPE_DEFAULT &&
      "@mozilla.org/toolkit/crash-reporter;1" in Components.classes) {
    
    
    let (crashReporter =
          Components.classes["@mozilla.org/toolkit/crash-reporter;1"]
          .getService(Components.interfaces.nsICrashReporter)) {
      crashReporter.enabled = true;
      crashReporter.minidumpPath = do_get_cwd();
    }
  }
}
catch (e) { }







const _timerFuzz = 15;

function _Timer(func, delay) {
  delay = Number(delay);
  if (delay < 0)
    do_throw("do_timeout() delay must be nonnegative");

  if (typeof func !== "function")
    do_throw("string callbacks no longer accepted; use a function!");

  this._func = func;
  this._start = Date.now();
  this._delay = delay;

  var timer = Components.classes["@mozilla.org/timer;1"]
                        .createInstance(Components.interfaces.nsITimer);
  timer.initWithCallback(this, delay + _timerFuzz, timer.TYPE_ONE_SHOT);

  
  _pendingTimers.push(timer);
}
_Timer.prototype = {
  QueryInterface: function(iid) {
    if (iid.Equals(Components.interfaces.nsITimerCallback) ||
        iid.Equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  notify: function(timer) {
    _pendingTimers.splice(_pendingTimers.indexOf(timer), 1);

    
    
    
    var end = Date.now();
    var elapsed = end - this._start;
    if (elapsed >= this._delay) {
      try {
        this._func.call(null);
      } catch (e) {
        do_throw("exception thrown from do_timeout callback: " + e);
      }
      return;
    }

    
    
    var newDelay = this._delay - elapsed;
    do_timeout(newDelay, this._func);
  }
};

function _do_main() {
  if (_quit)
    return;

  _dump("TEST-INFO | (xpcshell/head.js) | running event loop\n");

  var thr = Components.classes["@mozilla.org/thread-manager;1"]
                      .getService().currentThread;

  while (!_quit)
    thr.processNextEvent(true);

  while (thr.hasPendingEvents())
    thr.processNextEvent(true);
}

function _do_quit() {
  _dump("TEST-INFO | (xpcshell/head.js) | exiting test\n");

  _quit = true;
}

function _dump_exception_stack(stack) {
  stack.split("\n").forEach(function(frame) {
    if (!frame)
      return;
    
    let frame_regexp = new RegExp("(.*)\\(.*\\)@(.*):(\\d*)", "g");
    let parts = frame_regexp.exec(frame);
    if (parts)
        dump("JS frame :: " + parts[2] + " :: " + (parts[1] ? parts[1] : "anonymous")
             + " :: line " + parts[3] + "\n");
    else 
        dump("JS frame :: " + frame + "\n");
  });
}











_fakeIdleService = {
  get registrar() {
    delete this.registrar;
    return this.registrar =
      Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
  },
  contractID: "@mozilla.org/widget/idleservice;1",
  get CID() this.registrar.contractIDToCID(this.contractID),

  activate: function FIS_activate()
  {
    if (!this.originalFactory) {
      
      this.originalFactory =
        Components.manager.getClassObject(Components.classes[this.contractID],
                                          Components.interfaces.nsIFactory);
      
      this.registrar.unregisterFactory(this.CID, this.originalFactory);
      
      this.registrar.registerFactory(this.CID, "Fake Idle Service",
                                     this.contractID, this.factory
      );
    }
  },

  deactivate: function FIS_deactivate()
  {
    if (this.originalFactory) {
      
      this.registrar.unregisterFactory(this.CID, this.factory);
      
      this.registrar.registerFactory(this.CID, "Idle Service",
                                     this.contractID, this.originalFactory);
      delete this.originalFactory;
    }
  },

  factory: {
    
    createInstance: function (aOuter, aIID)
    {
      if (aOuter) {
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      }
      return _fakeIdleService.QueryInterface(aIID);
    },
    lockFactory: function (aLock) {
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
    },
    QueryInterface: function(aIID) {
      if (aIID.equals(Components.interfaces.nsIFactory) ||
          aIID.equals(Components.interfaces.nsISupports)) {
        return this;
      }
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  },

  
  get idleTime() 0,
  addIdleObserver: function () {},
  removeIdleObserver: function () {},

  QueryInterface: function(aIID) {
    
    if (aIID.equals(Components.interfaces.nsIFactory)) {
      return this.factory;
    }
    if (aIID.equals(Components.interfaces.nsIIdleService) ||
        aIID.equals(Components.interfaces.nsISupports)) {
      return this;
    }
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}





function do_get_idle() {
  _fakeIdleService.deactivate();
  return Components.classes[_fakeIdleService.contractID]
                   .getService(Components.interfaces.nsIIdleService);
}

function _execute_test() {
  
  let (ios = Components.classes["@mozilla.org/network/io-service;1"]
             .getService(Components.interfaces.nsIIOService)) {
    let protocolHandler =
      ios.getProtocolHandler("resource")
         .QueryInterface(Components.interfaces.nsIResProtocolHandler);
    let curDirURI = ios.newFileURI(do_get_cwd());
    protocolHandler.setSubstitution("test", curDirURI);
  }

  
  
  _fakeIdleService.activate();

  
  _load_files(_HEAD_FILES);
  
  _load_files(_TEST_FILE);

  try {
    do_test_pending();
    run_test();
    do_test_finished();
    _do_main();
  } catch (e) {
    _passed = false;
    
    
    
    
    
    if (!_quit || e != Components.results.NS_ERROR_ABORT) {
      msg = "TEST-UNEXPECTED-FAIL | (xpcshell/head.js) | " + e;
      if (e.stack) {
        _dump(msg + " - See following stack:\n");
        _dump_exception_stack(e.stack);
      }
      else {
        _dump(msg + "\n");
      }
    }
  }

  
  _load_files(_TAIL_FILES);

  
  var func;
  while ((func = _cleanupFunctions.pop()))
    func();

  
  _fakeIdleService.deactivate();

  if (!_passed)
    return;

  var truePassedChecks = _passedChecks - _falsePassedChecks;
  if (truePassedChecks > 0) {
    _dump("TEST-PASS | (xpcshell/head.js) | " + truePassedChecks + " (+ " +
            _falsePassedChecks + ") check(s) passed\n");
  } else {
    
    _dump("TEST-INFO | (xpcshell/head.js) | No (+ " + _falsePassedChecks + ") checks actually run\n");
  }
}






function _load_files(aFiles) {
  function loadTailFile(element, index, array) {
    load(element);
  }

  aFiles.forEach(loadTailFile);
}














function do_timeout(delay, func) {
  new _Timer(func, Number(delay));
}

function do_execute_soon(callback) {
  do_test_pending();
  var tm = Components.classes["@mozilla.org/thread-manager;1"]
                     .getService(Components.interfaces.nsIThreadManager);

  tm.mainThread.dispatch({
    run: function() {
      try {
        callback();
      } catch (e) {
        
        
        
        
        
        if (!_quit || e != Components.results.NS_ERROR_ABORT) {
          dump("TEST-UNEXPECTED-FAIL | (xpcshell/head.js) | " + e);
          if (e.stack) {
            dump(" - See following stack:\n");
            _dump_exception_stack(e.stack);
          }
          else {
            dump("\n");
          }
          _do_quit();
        }
      }
      finally {
        do_test_finished();
      }
    }
  }, Components.interfaces.nsIThread.DISPATCH_NORMAL);
}

function do_throw(text, stack) {
  if (!stack)
    stack = Components.stack.caller;

  _passed = false;
  _dump("TEST-UNEXPECTED-FAIL | " + stack.filename + " | " + text +
         " - See following stack:\n");
  var frame = Components.stack;
  while (frame != null) {
    _dump(frame + "\n");
    frame = frame.caller;
  }

  _do_quit();
  throw Components.results.NS_ERROR_ABORT;
}

function do_report_unexpected_exception(ex, text) {
  var caller_stack = Components.stack.caller;
  text = text ? text + " - " : "";

  _passed = false;
  dump("TEST-UNEXPECTED-FAIL | " + caller_stack.filename + " | " + text +
         "Unexpected exception " + ex + ", see following stack:\n" + ex.stack +
         "\n");

  _do_quit();
  throw Components.results.NS_ERROR_ABORT;
}

function do_note_exception(ex, text) {
  var caller_stack = Components.stack.caller;
  text = text ? text + " - " : "";

  dump("TEST-INFO | " + caller_stack.filename + " | " + text +
         "Swallowed exception " + ex + ", see following stack:\n" + ex.stack +
         "\n");
}

function do_check_neq(left, right, stack) {
  if (!stack)
    stack = Components.stack.caller;

  var text = left + " != " + right;
  if (left == right) {
    do_throw(text, stack);
  } else {
    ++_passedChecks;
    _dump("TEST-PASS | " + stack.filename + " | [" + stack.name + " : " +
         stack.lineNumber + "] " + text + "\n");
  }
}

function do_check_eq(left, right, stack) {
  if (!stack)
    stack = Components.stack.caller;

  var text = left + " == " + right;
  if (left != right) {
    do_throw(text, stack);
  } else {
    ++_passedChecks;
    _dump("TEST-PASS | " + stack.filename + " | [" + stack.name + " : " +
         stack.lineNumber + "] " + text + "\n");
  }
}

function do_check_true(condition, stack) {
  if (!stack)
    stack = Components.stack.caller;

  do_check_eq(condition, true, stack);
}

function do_check_false(condition, stack) {
  if (!stack)
    stack = Components.stack.caller;

  do_check_eq(condition, false, stack);
}

function do_test_pending() {
  ++_tests_pending;

  _dump("TEST-INFO | (xpcshell/head.js) | test " + _tests_pending +
         " pending\n");
}

function do_test_finished() {
  _dump("TEST-INFO | (xpcshell/head.js) | test " + _tests_pending +
         " finished\n");

  if (--_tests_pending == 0)
    _do_quit();
}

function do_get_file(path, allowNonexistent) {
  try {
    let lf = Components.classes["@mozilla.org/file/directory_service;1"]
      .getService(Components.interfaces.nsIProperties)
      .get("CurWorkD", Components.interfaces.nsILocalFile);

    let bits = path.split("/");
    for (let i = 0; i < bits.length; i++) {
      if (bits[i]) {
        if (bits[i] == "..")
          lf = lf.parent;
        else
          lf.append(bits[i]);
      }
    }

    if (!allowNonexistent && !lf.exists()) {
      
      _passed = false;
      var stack = Components.stack.caller;
      _dump("TEST-UNEXPECTED-FAIL | " + stack.filename + " | [" +
             stack.name + " : " + stack.lineNumber + "] " + lf.path +
             " does not exist\n");
    }

    return lf;
  }
  catch (ex) {
    do_throw(ex.toString(), Components.stack.caller);
  }

  return null;
}


function do_get_cwd() {
  return do_get_file("");
}





function do_load_httpd_js() {
  load(_HTTPD_JS_PATH);
}

function do_load_manifest(path) {
  var lf = do_get_file(path);
  const nsIComponentRegistrar = Components.interfaces.nsIComponentRegistrar;
  do_check_true(Components.manager instanceof nsIComponentRegistrar);
  
  ++_falsePassedChecks;
  Components.manager.autoRegister(lf);
}









function do_parse_document(aPath, aType) {
  switch (aType) {
    case "application/xhtml+xml":
    case "application/xml":
    case "text/xml":
      break;

    default:
      do_throw("type: expected application/xhtml+xml, application/xml or text/xml," +
                 " got '" + aType + "'",
               Components.stack.caller);
  }

  var lf = do_get_file(aPath);
  const C_i = Components.interfaces;
  const parserClass = "@mozilla.org/xmlextras/domparser;1";
  const streamClass = "@mozilla.org/network/file-input-stream;1";
  var stream = Components.classes[streamClass]
                         .createInstance(C_i.nsIFileInputStream);
  stream.init(lf, -1, -1, C_i.nsIFileInputStream.CLOSE_ON_EOF);
  var parser = Components.classes[parserClass]
                         .createInstance(C_i.nsIDOMParser);
  var doc = parser.parseFromStream(stream, null, lf.fileSize, aType);
  parser = null;
  stream = null;
  lf = null;
  return doc;
}








function do_register_cleanup(aFunction)
{
  _cleanupFunctions.push(aFunction);
}







function do_get_profile() {
  
  
  do_register_cleanup(function() {
    let obsSvc = Components.classes["@mozilla.org/observer-service;1"].
                 getService(Components.interfaces.nsIObserverService);
    obsSvc.notifyObservers(null, "profile-change-net-teardown", null);
    obsSvc.notifyObservers(null, "profile-change-teardown", null);
    obsSvc.notifyObservers(null, "profile-before-change", null);
  });

  let env = Components.classes["@mozilla.org/process/environment;1"]
                      .getService(Components.interfaces.nsIEnvironment);
  
  let profd = env.get("XPCSHELL_TEST_PROFILE_DIR");
  let file = Components.classes["@mozilla.org/file/local;1"]
                       .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath(profd);

  let dirSvc = Components.classes["@mozilla.org/file/directory_service;1"]
                         .getService(Components.interfaces.nsIProperties);
  let provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == "ProfD" || prop == "ProfLD" || prop == "ProfDS" ||
          prop == "ProfLDS" || prop == "TmpD") {
        return file.clone();
      }
      throw Components.results.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Components.interfaces.nsIDirectoryServiceProvider) ||
          iid.equals(Components.interfaces.nsISupports)) {
        return this;
      }
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
  };
  dirSvc.QueryInterface(Components.interfaces.nsIDirectoryService)
        .registerProvider(provider);
  return file.clone();
}









function do_load_child_test_harness()
{
  
  var runtime = Components.classes["@mozilla.org/xre/app-info;1"]
                  .getService(Components.interfaces.nsIXULRuntime);
  if (runtime.processType != 
            Components.interfaces.nsIXULRuntime.PROCESS_TYPE_DEFAULT) 
  {
    do_throw("run_test_in_child cannot be called from child!");
  }

  
  if (typeof do_load_child_test_harness.alreadyRun != "undefined")
    return;
  do_load_child_test_harness.alreadyRun = 1;
  
  function addQuotes (str)  { 
    return '"' + str + '"'; 
  }
  var quoted_head_files = _HEAD_FILES.map(addQuotes);
  var quoted_tail_files = _TAIL_FILES.map(addQuotes);

  _XPCSHELL_PROCESS = "parent";
 
  sendCommand(
        "const _HEAD_JS_PATH='" + _HEAD_JS_PATH + "'; "
      + "const _HTTPD_JS_PATH='" + _HTTPD_JS_PATH + "'; "
      + "const _HEAD_FILES=[" + quoted_head_files.join() + "];"
      + "const _TAIL_FILES=[" + quoted_tail_files.join() + "];"
      + "const _XPCSHELL_PROCESS='child';"
      + "load(_HEAD_JS_PATH);");
}













function run_test_in_child(testFile, optionalCallback) 
{
  var callback = (typeof optionalCallback == 'undefined') ? 
                    do_test_finished : optionalCallback;

  do_load_child_test_harness();

  var testPath = do_get_file(testFile).path.replace(/\\/g, "/");
  do_test_pending();
  sendCommand("const _TEST_FILE=['" + testPath + "']; _execute_test();", 
              callback);
}

