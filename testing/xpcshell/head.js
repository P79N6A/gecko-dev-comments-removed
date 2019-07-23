













































var _quit = false;
var _fail = false;
var _tests_pending = 0;



let (ios = Components.classes["@mozilla.org/network/io-service;1"]
           .getService(Components.interfaces.nsIIOService2)) {
  ios.manageOfflineStatus = false;
  ios.offline = false;
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

  dump("*** running event loop\n");
  var thr = Components.classes["@mozilla.org/thread-manager;1"]
                      .getService().currentThread;

  while (!_quit)
    thr.processNextEvent(true);

  while (thr.hasPendingEvents())
    thr.processNextEvent(true);
}

function _do_quit() {
  dump("*** exiting\n");

  _quit = true;
}

function _execute_test() {
  try {
    do_test_pending();
    run_test();
    do_test_finished();
    _do_main();
  } catch (e) {
    _fail = true;
    dump(e + "\n");
  }

  if (_fail)
    dump("*** FAIL ***\n");
  else
    dump("*** PASS ***\n");
}





function do_timeout(delay, expr) {
  var timer = Components.classes["@mozilla.org/timer;1"]
                        .createInstance(Components.interfaces.nsITimer);
  timer.initWithCallback(new _TimerCallback(expr), delay, timer.TYPE_ONE_SHOT);
}

function do_throw(text, stack) {
  if (!stack)
    stack = Components.stack.caller;
  _fail = true;
  _do_quit();
  dump("*** TEST-UNEXPECTED-FAIL | " + stack.filename + " | " + text + "\n");
  var frame = Components.stack;
  while (frame != null) {
    dump(frame + "\n");
    frame = frame.caller;
  }
  throw Components.results.NS_ERROR_ABORT;
}

function do_check_neq(left, right, stack) {
  if (!stack)
    stack = Components.stack.caller;
  if (left == right)
    do_throw(left + " != " + right, stack);
}

function do_check_eq(left, right, stack) {
  if (!stack)
    stack = Components.stack.caller;
  if (left != right)
    do_throw(left + " == " + right, stack);
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
  dump("*** test pending\n");
  _tests_pending++;
}

function do_test_finished() {
  dump("*** test finished\n");
  if (--_tests_pending == 0)
    _do_quit();
}

function do_import_script(topsrcdirRelativePath) {
  var scriptPath = environment["TOPSRCDIR"];
  if (scriptPath.charAt(scriptPath.length - 1) != "/")
    scriptPath += "/";
  scriptPath += topsrcdirRelativePath;

  load(scriptPath);
}

function do_get_file(path, allowInexistent) {
  var comps = path.split("/");
  try {
    
    
    var lf = Components.classes["@mozilla.org/file/local;1"]
                       .createInstance(Components.interfaces.nsILocalFile);
    lf.initWithPath(environment["NATIVE_TOPSRCDIR"]);
  } catch (e) {
    
    lf = Components.classes["@mozilla.org/file/directory_service;1"]
                   .getService(Components.interfaces.nsIProperties)
                   .get("CurWorkD", Components.interfaces.nsILocalFile);

    
    
    var topsrcdirComps = environment["NATIVE_TOPSRCDIR"].split("/");
    Array.prototype.unshift.apply(comps, topsrcdirComps);
  }

  for (var i = 0, sz = comps.length; i < sz; i++) {
    
    if (comps[i].length > 0) {
      if (comps[i] == "..")
        lf = lf.parent;
      else
        lf.append(comps[i]);
    }
  }

  if (!allowInexistent) {
    if (!lf.exists()) {
      print(lf.path + " doesn't exist\n");
    }
    do_check_true(lf.exists());
  }

  return lf;
}

function do_load_module(path) {
  var lf = do_get_file(path);
  const nsIComponentRegistrar = Components.interfaces.nsIComponentRegistrar;
  do_check_true(Components.manager instanceof nsIComponentRegistrar);
  Components.manager.autoRegister(lf);
}









function do_parse_document(aPath, aType) {
  switch (aType) {
    case "application/xhtml+xml":
    case "application/xml":
    case "text/xml":
      break;

    default:
      throw new Error("do_parse_document requires content-type of " +
                      "application/xhtml+xml, application/xml, or text/xml.");
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
