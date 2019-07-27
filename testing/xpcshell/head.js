











var _quit = false;
var _passed = true;
var _tests_pending = 0;
var _passedChecks = 0, _falsePassedChecks = 0;
var _todoChecks = 0;
var _cleanupFunctions = [];
var _pendingTimers = [];
var _profileInitialized = false;



_register_modules_protocol_handler();

let _Promise = Components.utils.import("resource://gre/modules/Promise.jsm", this).Promise;


let AssertCls = Components.utils.import("resource://testing-common/Assert.jsm", null).Assert;

let Assert = new AssertCls(function(err, message, stack) {
  if (err) {
    do_report_result(false, err.message, err.stack);
  } else {
    do_report_result(true, message, stack);
  }
});

let _log = function (action, params) {
  if (typeof _XPCSHELL_PROCESS != "undefined") {
    params.process = _XPCSHELL_PROCESS;
  }
  params.action = action;
  params._time = Date.now();
  dump("\n" + JSON.stringify(params) + "\n");
}

function _dump(str) {
  let start = /^TEST-/.test(str) ? "\n" : "";
  if (typeof _XPCSHELL_PROCESS == "undefined") {
    dump(start + str);
  } else {
    dump(start + _XPCSHELL_PROCESS + ": " + str);
  }
}



let (ios = Components.classes["@mozilla.org/network/io-service;1"]
           .getService(Components.interfaces.nsIIOService2)) {
  ios.manageOfflineStatus = false;
  ios.offline = false;
}


let runningInParent = true;
try {
  runningInParent = Components.classes["@mozilla.org/xre/runtime;1"].
                    getService(Components.interfaces.nsIXULRuntime).processType
                    == Components.interfaces.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
} 
catch (e) { }


if (runningInParent &&
    "mozIAsyncHistory" in Components.interfaces) {
  
  
  let (prefs = Components.classes["@mozilla.org/preferences-service;1"]
               .getService(Components.interfaces.nsIPrefBranch)) {
    prefs.setBoolPref("places.history.enabled", true);
  };
}

try {
  if (runningInParent) {
    let prefs = Components.classes["@mozilla.org/preferences-service;1"]
                .getService(Components.interfaces.nsIPrefBranch);

    
    
    prefs.setBoolPref("network.disable.ipc.security", true);

    
    if ("@mozilla.org/windows-registry-key;1" in Components.classes) {
      prefs.setCharPref("network.dns.ipv4OnlyDomains", "localhost");
    }
  }
}
catch (e) { }






try {
  if (runningInParent &&
      "@mozilla.org/toolkit/crash-reporter;1" in Components.classes) {
    let (crashReporter =
          Components.classes["@mozilla.org/toolkit/crash-reporter;1"]
          .getService(Components.interfaces.nsICrashReporter)) {
      crashReporter.UpdateCrashEventsDir();
      crashReporter.minidumpPath = do_get_minidumpdir();
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
    if (iid.equals(Components.interfaces.nsITimerCallback) ||
        iid.equals(Components.interfaces.nsISupports))
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

  _log("test_info",
       {_message: "TEST-INFO | (xpcshell/head.js) | running event loop\n"});

  var thr = Components.classes["@mozilla.org/thread-manager;1"]
                      .getService().currentThread;

  while (!_quit)
    thr.processNextEvent(true);

  while (thr.hasPendingEvents())
    thr.processNextEvent(true);
}

function _do_quit() {
  _log("test_info",
       {_message: "TEST-INFO | (xpcshell/head.js) | exiting test\n"});
  _Promise.Debugging.flushUncaughtErrors();
  _quit = true;
}

function _format_exception_stack(stack) {
  if (typeof stack == "object" && stack.caller) {
    let frame = stack;
    let strStack = "";
    while (frame != null) {
      strStack += frame + "\n";
      frame = frame.caller;
    }
    stack = strStack;
  }
  
  let frame_regexp = new RegExp("(.*)@(.*):(\\d*)", "g");
  return stack.split("\n").reduce(function(stack_msg, frame) {
    if (frame) {
      let parts = frame_regexp.exec(frame);
      if (parts) {
        let [ _, func, file, line ] = parts;
        return stack_msg + "JS frame :: " + file + " :: " +
          (func || "anonymous") + " :: line " + line + "\n";
      }
      else { 
        return stack_msg + "JS frame :: " + frame + "\n";
      }
    }
    return stack_msg;
  }, "");
}











var _fakeIdleService = {
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



function _register_protocol_handlers() {
  let ios = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);
  let protocolHandler =
    ios.getProtocolHandler("resource")
       .QueryInterface(Components.interfaces.nsIResProtocolHandler);

  let curDirURI = ios.newFileURI(do_get_cwd());
  protocolHandler.setSubstitution("test", curDirURI);

  _register_modules_protocol_handler();
}

function _register_modules_protocol_handler() {
  if (!this._TESTING_MODULES_DIR) {
    throw new Error("Please define a path where the testing modules can be " +
                    "found in a variable called '_TESTING_MODULES_DIR' before " +
                    "head.js is included.");
  }

  let ios = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);
  let protocolHandler =
    ios.getProtocolHandler("resource")
       .QueryInterface(Components.interfaces.nsIResProtocolHandler);

  let modulesFile = Components.classes["@mozilla.org/file/local;1"].
                    createInstance(Components.interfaces.nsILocalFile);
  modulesFile.initWithPath(_TESTING_MODULES_DIR);

  if (!modulesFile.exists()) {
    throw new Error("Specified modules directory does not exist: " +
                    _TESTING_MODULES_DIR);
  }

  if (!modulesFile.isDirectory()) {
    throw new Error("Specified modules directory is not a directory: " +
                    _TESTING_MODULES_DIR);
  }

  let modulesURI = ios.newFileURI(modulesFile);

  protocolHandler.setSubstitution("testing-common", modulesURI);
}

function _execute_test() {
  _register_protocol_handlers();

  
  
  _fakeIdleService.activate();

  _Promise.Debugging.clearUncaughtErrorObservers();
  _Promise.Debugging.addUncaughtErrorObserver(function observer({message, date, fileName, stack, lineNumber}) {
    let text = " A promise chain failed to handle a rejection: " +
        message + " - rejection date: " + date;
    _log_message_with_stack("test_unexpected_fail",
                            text, stack, fileName);
  });

  
  _load_files(_HEAD_FILES);
  
  _load_files(_TEST_FILE);

  
  this.Assert = Assert;
  for (let func in Assert) {
    this[func] = Assert[func].bind(Assert);
  }

  try {
    do_test_pending("MAIN run_test");
    run_test();
    do_test_finished("MAIN run_test");
    _do_main();
  } catch (e) {
    _passed = false;
    
    
    
    
    
    if (!_quit || e != Components.results.NS_ERROR_ABORT) {
      let msgObject = {};
      if (e.fileName) {
        msgObject.source_file = e.fileName;
        if (e.lineNumber) {
          msgObject.line_number = e.lineNumber;
        }
      } else {
        msgObject.source_file = "xpcshell/head.js";
      }
      msgObject.diagnostic = _exception_message(e);
      if (e.stack) {
        msgObject.diagnostic += " - See following stack:\n";
        msgObject.stack = _format_exception_stack(e.stack);
      }
      _log("test_unexpected_fail", msgObject);
    }
  }

  
  _load_files(_TAIL_FILES);

  
  let reportCleanupError = function(ex) {
    let stack, filename;
    if (ex && typeof ex == "object" && "stack" in ex) {
      stack = ex.stack;
    } else {
      stack = Components.stack.caller;
    }
    if (stack instanceof Components.interfaces.nsIStackFrame) {
      filename = stack.filename;
    } else if (ex.fileName) {
      filename = ex.fileName;
    }
    _log_message_with_stack("test_unexpected_fail",
                            ex, stack, filename);
  };

  let func;
  while ((func = _cleanupFunctions.pop())) {
    let result;
    try {
      result = func();
    } catch (ex) {
      reportCleanupError(ex);
      continue;
    }
    if (result && typeof result == "object"
        && "then" in result && typeof result.then == "function") {
      
      let complete = false;
      let promise = result.then(null, reportCleanupError);
      promise = promise.then(() => complete = true);
      let thr = Components.classes["@mozilla.org/thread-manager;1"]
                  .getService().currentThread;
      while (!complete) {
        thr.processNextEvent(true);
      }
    }
  }

  
  _fakeIdleService.deactivate();

  if (!_passed)
    return;

  var truePassedChecks = _passedChecks - _falsePassedChecks;
  if (truePassedChecks > 0) {
    _log("test_pass",
         {_message: "TEST-PASS | (xpcshell/head.js) | " + truePassedChecks + " (+ " +
                    _falsePassedChecks + ") check(s) passed\n",
          source_file: _TEST_FILE,
          passed_checks: truePassedChecks});
    _log("test_info",
         {_message: "TEST-INFO | (xpcshell/head.js) | " + _todoChecks +
                    " check(s) todo\n",
          source_file: _TEST_FILE,
          todo_checks: _todoChecks});
  } else {
    
    _log("test_info",
         {_message: "TEST-INFO | (xpcshell/head.js) | No (+ " + _falsePassedChecks +
                    ") checks actually run\n",
         source_file: _TEST_FILE});
  }
}






function _load_files(aFiles) {
  function loadTailFile(element, index, array) {
    try {
      load(element);
    } catch (e if e instanceof SyntaxError) {
      _log("javascript_error",
           {_message: "TEST-UNEXPECTED-FAIL | (xpcshell/head.js) | Source file " + element + " contains SyntaxError",
            diagnostic: _exception_message(e),
            source_file: element,
            stack: _format_exception_stack(e.stack)});
    } catch (e) {
      _log("javascript_error",
           {_message: "TEST-UNEXPECTED-FAIL | (xpcshell/head.js) | Source file " + element + " contains an error",
            diagnostic: _exception_message(e),
            source_file: element,
            stack: e.stack ? _format_exception_stack(e.stack) : null});
    }
  }

  aFiles.forEach(loadTailFile);
}

function _wrap_with_quotes_if_necessary(val) {
  return typeof val == "string" ? '"' + val + '"' : val;
}






function do_print(msg) {
  var caller_stack = Components.stack.caller;
  msg = _wrap_with_quotes_if_necessary(msg);
  _log("test_info",
       {source_file: caller_stack.filename,
        diagnostic: msg});

}











function do_timeout(delay, func) {
  new _Timer(func, Number(delay));
}

function do_execute_soon(callback, aName) {
  let funcName = (aName ? aName : callback.name);
  do_test_pending(funcName);
  var tm = Components.classes["@mozilla.org/thread-manager;1"]
                     .getService(Components.interfaces.nsIThreadManager);

  tm.mainThread.dispatch({
    run: function() {
      try {
        callback();
      } catch (e) {
        
        
        
        
        
        if (!_quit || e != Components.results.NS_ERROR_ABORT) {
          if (e.stack) {
            _log("javascript_error",
                 {source_file: "xpcshell/head.js",
                  diagnostic: _exception_message(e) + " - See following stack:",
                  stack: _format_exception_stack(e.stack)});
          } else {
            _log("javascript_error",
                 {source_file: "xpcshell/head.js",
                  diagnostic: _exception_message(e)});
          }
          _do_quit();
        }
      }
      finally {
        do_test_finished(funcName);
      }
    }
  }, Components.interfaces.nsIThread.DISPATCH_NORMAL);
}








function do_throw(error, stack) {
  let filename = "";
  
  
  stack = stack || error.stack || Components.stack.caller;

  if (stack instanceof Components.interfaces.nsIStackFrame)
    filename = stack.filename;
  else if (error.fileName)
    filename = error.fileName;

  _log_message_with_stack("test_unexpected_fail",
                          error, stack, filename);

  _passed = false;
  _do_quit();
  throw Components.results.NS_ERROR_ABORT;
}

function _format_stack(stack) {
  let normalized;
  if (stack instanceof Components.interfaces.nsIStackFrame) {
    let frames = [];
    for (let frame = stack; frame; frame = frame.caller) {
      frames.push(frame.filename + ":" + frame.name + ":" + frame.lineNumber);
    }
    normalized = frames.join("\n");
  } else {
    normalized = "" + stack;
  }
  return _Task.Debugging.generateReadableStack(normalized, "    ");
}

function do_throw_todo(text, stack) {
  if (!stack)
    stack = Components.stack.caller;

  _passed = false;
  _log_message_with_stack("test_unexpected_pass",
                          text, stack, stack.filename);
  _do_quit();
  throw Components.results.NS_ERROR_ABORT;
}



function _exception_message(ex) {
  let message = "";
  if (ex.name) {
    message = ex.name + ": ";
  }
  if (ex.message) {
    message += ex.message;
  }
  if (ex.fileName) {
    message += (" at " + ex.fileName);
    if (ex.lineNumber) {
      message += (":" + ex.lineNumber);
    }
  }
  if (message !== "") {
    return message;
  }
  
  return "" + ex;
}

function _log_message_with_stack(action, ex, stack, filename, text) {
  if (stack) {
    _log(action,
         {diagnostic: (text ? text : "") +
                      _exception_message(ex) +
                      " - See following stack:",
          source_file: filename,
          stack: _format_stack(stack)});
  } else {
    _log(action,
         {diagnostic: (text ? text : "") +
                      _exception_message(ex),
          source_file: filename});
  }
}

function do_report_unexpected_exception(ex, text) {
  var caller_stack = Components.stack.caller;
  text = text ? text + " - " : "";

  _passed = false;
  _log_message_with_stack("test_unexpected_fail", ex, ex.stack || "",
                          caller_stack.filename, text + "Unexpected exception ");
  _do_quit();
  throw Components.results.NS_ERROR_ABORT;
}

function do_note_exception(ex, text) {
  var caller_stack = Components.stack.caller;
  text = text ? text + " - " : "";

  _log_message_with_stack("test_info", ex, ex.stack,
                          caller_stack.filename, text + "Swallowed exception ");
}

function _do_check_neq(left, right, stack, todo) {
  Assert.notEqual(left, right);
}

function do_check_neq(left, right, stack) {
  if (!stack)
    stack = Components.stack.caller;

  _do_check_neq(left, right, stack, false);
}

function todo_check_neq(left, right, stack) {
  if (!stack)
      stack = Components.stack.caller;

  _do_check_neq(left, right, stack, true);
}

function do_report_result(passed, text, stack, todo) {
  while (stack.filename.contains("head.js") && stack.caller) {
    stack = stack.caller;
  }
  if (passed) {
    if (todo) {
      do_throw_todo(text, stack);
    } else {
      ++_passedChecks;
      _log("test_pass",
           {source_file: stack.filename,
            test_name: stack.name,
            line_number: stack.lineNumber,
            diagnostic: "[" + stack.name + " : " + stack.lineNumber + "] " +
                        text + "\n"});
    }
  } else {
    if (todo) {
      ++_todoChecks;
      _log("test_known_fail",
           {source_file: stack.filename,
            test_name: stack.name,
            line_number: stack.lineNumber,
            diagnostic: "[" + stack.name + " : " + stack.lineNumber + "] " +
                        text + "\n"});
    } else {
      do_throw(text, stack);
    }
  }
}

function _do_check_eq(left, right, stack, todo) {
  if (!stack)
    stack = Components.stack.caller;

  var text = _wrap_with_quotes_if_necessary(left) + " == " +
             _wrap_with_quotes_if_necessary(right);
  do_report_result(left == right, text, stack, todo);
}

function do_check_eq(left, right, stack) {
  Assert.equal(left, right);
}

function todo_check_eq(left, right, stack) {
  if (!stack)
      stack = Components.stack.caller;

  _do_check_eq(left, right, stack, true);
}

function do_check_true(condition, stack) {
  Assert.ok(condition);
}

function todo_check_true(condition, stack) {
  if (!stack)
    stack = Components.stack.caller;

  todo_check_eq(condition, true, stack);
}

function do_check_false(condition, stack) {
  Assert.ok(!condition, stack);
}

function todo_check_false(condition, stack) {
  if (!stack)
    stack = Components.stack.caller;

  todo_check_eq(condition, false, stack);
}

function do_check_null(condition, stack) {
  Assert.equal(condition, null);
}

function todo_check_null(condition, stack=Components.stack.caller) {
  todo_check_eq(condition, null, stack);
}
function do_check_matches(pattern, value) {
  Assert.deepEqual(pattern, value);
}



function do_check_throws_nsIException(func, resultName,
                                      stack=Components.stack.caller, todo=false)
{
  let expected = Components.results[resultName];
  if (typeof expected !== 'number') {
    do_throw("do_check_throws_nsIException requires a Components.results" +
             " property name, not " + uneval(resultName), stack);
  }

  let msg = ("do_check_throws_nsIException: func should throw" +
             " an nsIException whose 'result' is Components.results." +
             resultName);

  try {
    func();
  } catch (ex) {
    if (!(ex instanceof Components.interfaces.nsIException) ||
        ex.result !== expected) {
      do_report_result(false, msg + ", threw " + legible_exception(ex) +
                       " instead", stack, todo);
    }

    do_report_result(true, msg, stack, todo);
    return;
  }

  
  
  do_report_result(false, msg + ", but returned normally", stack, todo);
}



function legible_exception(exception)
{
  switch (typeof exception) {
    case 'object':
    if (exception instanceof Components.interfaces.nsIException) {
      return "nsIException instance: " + uneval(exception.toString());
    }
    return exception.toString();

    case 'number':
    for (let name in Components.results) {
      if (exception === Components.results[name]) {
        return "Components.results." + name;
      }
    }

    
    default:
    return uneval(exception);
  }
}

function do_check_instanceof(value, constructor,
                             stack=Components.stack.caller, todo=false) {
  do_report_result(value instanceof constructor,
                   "value should be an instance of " + constructor.name,
                   stack, todo);
}

function todo_check_instanceof(value, constructor,
                             stack=Components.stack.caller) {
  do_check_instanceof(value, constructor, stack, true);
}

function do_test_pending(aName) {
  ++_tests_pending;

  _log("test_pending",
       {_message: "TEST-INFO | (xpcshell/head.js) | test" +
                  (aName ? " " + aName : "") +
                  " pending (" + _tests_pending + ")\n"});
}

function do_test_finished(aName) {
  _log("test_finish",
       {_message: "TEST-INFO | (xpcshell/head.js) | test" +
                  (aName ? " " + aName : "") +
                  " finished (" + _tests_pending + ")\n"});
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
      _log("test_unexpected_fail",
           {source_file: stack.filename,
            test_name: stack.name,
            line_number: stack.lineNumber,
            diagnostic: "[" + stack.name + " : " + stack.lineNumber + "] " +
                        lf.path + " does not exist\n"});
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







function do_get_tempdir() {
  let env = Components.classes["@mozilla.org/process/environment;1"]
                      .getService(Components.interfaces.nsIEnvironment);
  
  let path = env.get("XPCSHELL_TEST_TEMP_DIR");
  let file = Components.classes["@mozilla.org/file/local;1"]
                       .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath(path);
  return file;
}






function do_get_minidumpdir() {
  let env = Components.classes["@mozilla.org/process/environment;1"]
                      .getService(Components.interfaces.nsIEnvironment);
  
  let path = env.get("XPCSHELL_MINIDUMP_DIR");
  if (path) {
    let file = Components.classes["@mozilla.org/file/local;1"]
                         .createInstance(Components.interfaces.nsILocalFile);
    file.initWithPath(path);
    return file;
  } else {
    return do_get_tempdir();
  }
}







function do_get_profile() {
  if (!runningInParent) {
    _log("test_info",
         {_message: "TEST-INFO | (xpcshell/head.js) | Ignoring profile creation from child process.\n"});

    return null;
  }

  if (!_profileInitialized) {
    
    
    do_register_cleanup(function() {
      let obsSvc = Components.classes["@mozilla.org/observer-service;1"].
                   getService(Components.interfaces.nsIObserverService);
      obsSvc.notifyObservers(null, "profile-change-net-teardown", null);
      obsSvc.notifyObservers(null, "profile-change-teardown", null);
      obsSvc.notifyObservers(null, "profile-before-change", null);
    });
  }

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
      return null;
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

  let obsSvc = Components.classes["@mozilla.org/observer-service;1"].
        getService(Components.interfaces.nsIObserverService);

  
  if (runningInParent &&
      "@mozilla.org/toolkit/crash-reporter;1" in Components.classes) {
    let crashReporter =
        Components.classes["@mozilla.org/toolkit/crash-reporter;1"]
                          .getService(Components.interfaces.nsICrashReporter);
    crashReporter.UpdateCrashEventsDir();
  }

  if (!_profileInitialized) {
    obsSvc.notifyObservers(null, "profile-do-change", "xpcshell-do-get-profile");
    _profileInitialized = true;
  }

  
  
  env = null;
  profd = null;
  dirSvc = null;
  provider = null;
  obsSvc = null;
  return file.clone();
}









function do_load_child_test_harness()
{
  
  if (!runningInParent) {
    do_throw("run_test_in_child cannot be called from child!");
  }

  
  if (typeof do_load_child_test_harness.alreadyRun != "undefined")
    return;
  do_load_child_test_harness.alreadyRun = 1;

  _XPCSHELL_PROCESS = "parent";

  let command =
        "const _HEAD_JS_PATH=" + uneval(_HEAD_JS_PATH) + "; "
      + "const _HTTPD_JS_PATH=" + uneval(_HTTPD_JS_PATH) + "; "
      + "const _HEAD_FILES=" + uneval(_HEAD_FILES) + "; "
      + "const _TAIL_FILES=" + uneval(_TAIL_FILES) + "; "
      + "const _XPCSHELL_PROCESS='child';";

  if (this._TESTING_MODULES_DIR) {
    command += " const _TESTING_MODULES_DIR=" + uneval(_TESTING_MODULES_DIR) + ";";
  }

  command += " load(_HEAD_JS_PATH);";
  sendCommand(command);
}













function run_test_in_child(testFile, optionalCallback) 
{
  var callback = (typeof optionalCallback == 'undefined') ? 
                    do_test_finished : optionalCallback;

  do_load_child_test_harness();

  var testPath = do_get_file(testFile).path.replace(/\\/g, "/");
  do_test_pending("run in child");
  sendCommand("_log('child_test_start', {_message: 'CHILD-TEST-STARTED'}); "
              + "const _TEST_FILE=['" + testPath + "']; _execute_test(); "
              + "_log('child_test_end', {_message: 'CHILD-TEST-COMPLETED'});",
              callback);
}





function do_await_remote_message(name, callback)
{
  var listener = {
    receiveMessage: function(message) {
      if (message.name == name) {
        mm.removeMessageListener(name, listener);
        callback();
        do_test_finished();
      }
    }
  };

  var mm;
  if (runningInParent) {
    mm = Cc["@mozilla.org/parentprocessmessagemanager;1"].getService(Ci.nsIMessageBroadcaster);
  } else {
    mm = Cc["@mozilla.org/childprocessmessagemanager;1"].getService(Ci.nsISyncMessageSender);
  }
  do_test_pending();
  mm.addMessageListener(name, listener);
}





function do_send_remote_message(name) {
  var mm;
  var sender;
  if (runningInParent) {
    mm = Cc["@mozilla.org/parentprocessmessagemanager;1"].getService(Ci.nsIMessageBroadcaster);
    sender = 'broadcastAsyncMessage';
  } else {
    mm = Cc["@mozilla.org/childprocessmessagemanager;1"].getService(Ci.nsISyncMessageSender);
    sender = 'sendAsyncMessage';
  }
  mm[sender](name);
}










let _gTests = [];
function add_test(func) {
  _gTests.push([false, func]);
  return func;
}






































function add_task(func) {
  _gTests.push([true, func]);
}
let _Task = Components.utils.import("resource://gre/modules/Task.jsm", {}).Task;
_Task.Debugging.maintainStack = true;





let _gRunningTest = null;
let _gTestIndex = 0; 
let _gTaskRunning = false;
function run_next_test()
{
  if (_gTaskRunning) {
    throw new Error("run_next_test() called from an add_task() test function. " +
                    "run_next_test() should not be called from inside add_task() " +
                    "under any circumstances!");
  }
 
  function _run_next_test()
  {
    if (_gTestIndex < _gTests.length) {
      
      _Promise.Debugging.flushUncaughtErrors();
      let _isTask;
      [_isTask, _gRunningTest] = _gTests[_gTestIndex++];
      print("TEST-INFO | " + _TEST_FILE + " | Starting " + _gRunningTest.name);
      do_test_pending(_gRunningTest.name);

      if (_isTask) {
        _gTaskRunning = true;
        _Task.spawn(_gRunningTest).then(
          () => { _gTaskRunning = false; run_next_test(); },
          (ex) => { _gTaskRunning = false; do_report_unexpected_exception(ex); }
        );
      } else {
        
        try {
          _gRunningTest();
        } catch (e) {
          do_throw(e);
        }
      }
    }
  }

  
  
  
  
  do_execute_soon(_run_next_test, "run_next_test " + _gTestIndex);

  if (_gRunningTest !== null) {
    
    do_test_finished(_gRunningTest.name);
  }
}

try {
  if (runningInParent) {
    
    
    let prefs = Components.classes["@mozilla.org/preferences-service;1"]
      .getService(Components.interfaces.nsIPrefBranch);

    prefs.setBoolPref("geo.provider.testing", true);
  }
} catch (e) { }


try {
  if (runningInParent) {
    let prefs = Components.classes["@mozilla.org/preferences-service;1"]
      .getService(Components.interfaces.nsIPrefBranch);

    prefs.setCharPref("media.gmp-manager.url.override", "http://%(server)s/dummy-gmp-manager.xml");
  }
} catch (e) { }
