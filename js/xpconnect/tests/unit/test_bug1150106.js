



const Cc = Components.classes;
const Ci = Components.interfaces;

var srvScope = {};

function success(result) {
  equal(result, 42, "Result of script is correct");
  ok('makeTags' in srvScope && srvScope.makeTags instanceof Function,
     "makeTags is a function.");
  do_test_finished();
}

function error() {
  ok(false, "error loading the script asynchronously.");
  do_test_finished();
}

function run_test() {
  do_test_pending();

  var file = do_get_file("bug451678_subscript.js");
  var ios = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService);
  var uri = ios.newFileURI(file);
  var scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
                       .getService(Ci.mozIJSSubScriptLoader);
  var p = scriptLoader.loadSubScriptWithOptions(uri.spec,
                                                { target: srvScope,
                                                  async: true });
  p.then(success, error);
}
