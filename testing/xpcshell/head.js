














































var _quit = false;
var _passed = true;
var _tests_pending = 0;
var _passedChecks = 0, _falsePassedChecks = 0;



let (ios = Components.classes["@mozilla.org/network/io-service;1"]
           .getService(Components.interfaces.nsIIOService2)) {
  ios.manageOfflineStatus = false;
  ios.offline = false;
}




if ("@mozilla.org/toolkit/crash-reporter;1" in Components.classes) {
  let (crashReporter =
        Components.classes["@mozilla.org/toolkit/crash-reporter;1"]
        .getService(Components.interfaces.nsICrashReporter)) {
    crashReporter.enabled = true;
    crashReporter.minidumpPath = do_get_cwd();
  }
}

function _TimerCallback(expr) {
  this._expr = expr;
}
_TimerCallback.prototype = {
  _expr: "",
  QueryInterface: function(iid) {
    if (iid.Equals(Components.interfaces.nsITimerCallback) ||
        iid.Equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  notify: function(timer) {
    eval(this._expr);
  }
};

function _do_main() {
  if (_quit)
    return;

  dump("TEST-INFO | (xpcshell/head.js) | running event loop\n");

  var thr = Components.classes["@mozilla.org/thread-manager;1"]
                      .getService().currentThread;

  while (!_quit)
    thr.processNextEvent(true);

  while (thr.hasPendingEvents())
    thr.processNextEvent(true);
}

function _do_quit() {
  dump("TEST-INFO | (xpcshell/head.js) | exiting test\n");

  _quit = true;
}

function _execute_test() {
  try {
    do_test_pending();
    run_test();
    do_test_finished();
    _do_main();
  } catch (e) {
    _passed = false;
    
    
    if (!_quit || e != Components.results.NS_ERROR_ABORT)
      dump("TEST-UNEXPECTED-FAIL | (xpcshell/head.js) | " + e + "\n");
  }

  if (!_passed)
    return;

  var truePassedChecks = _passedChecks - _falsePassedChecks;
  if (truePassedChecks > 0)
    dump("TEST-PASS | (xpcshell/head.js) | " + truePassedChecks + " (+ " +
            _falsePassedChecks + ") check(s) passed\n");
  else
    
    dump("TEST-INFO | (xpcshell/head.js) | No (+ " + _falsePassedChecks + ") checks actually run\n");
}





function do_timeout(delay, expr) {
  var timer = Components.classes["@mozilla.org/timer;1"]
                        .createInstance(Components.interfaces.nsITimer);
  timer.initWithCallback(new _TimerCallback(expr), delay, timer.TYPE_ONE_SHOT);
}

function do_throw(text, stack) {
  if (!stack)
    stack = Components.stack.caller;

  _passed = false;
  dump("TEST-UNEXPECTED-FAIL | " + stack.filename + " | " + text +
         " - See following stack:\n");
  var frame = Components.stack;
  while (frame != null) {
    dump(frame + "\n");
    frame = frame.caller;
  }

  _do_quit();
  throw Components.results.NS_ERROR_ABORT;
}

function do_check_neq(left, right, stack) {
  if (!stack)
    stack = Components.stack.caller;

  var text = left + " != " + right;
  if (left == right)
    do_throw(text, stack);
  else {
    ++_passedChecks;
    dump("TEST-PASS | " + stack.filename + " | [" + stack.name + " : " +
         stack.lineNumber + "] " + text + "\n");
  }
}

function do_check_eq(left, right, stack) {
  if (!stack)
    stack = Components.stack.caller;

  var text = left + " == " + right;
  if (left != right)
    do_throw(text, stack);
  else {
    ++_passedChecks;
    dump("TEST-PASS | " + stack.filename + " | [" + stack.name + " : " +
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

  dump("TEST-INFO | (xpcshell/head.js) | test " + _tests_pending +
         " pending\n");
}

function do_test_finished() {
  dump("TEST-INFO | (xpcshell/head.js) | test " + _tests_pending +
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
      dump("TEST-UNEXPECTED-FAIL | " + stack.filename + " | [" +
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

function do_load_module(path) {
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
