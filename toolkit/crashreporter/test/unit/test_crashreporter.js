function run_test()
{
  if (!("@mozilla.org/toolkit/crash-reporter;1" in Components.classes)) {
    dump("INFO | test_crashreporter.js | Can't test crashreporter in a non-libxul build.\n");
    return;
  }

  dump("INFO | test_crashreporter.js | Get crashreporter service.\n");
  var cr = Components.classes["@mozilla.org/toolkit/crash-reporter;1"]
                     .getService(Components.interfaces.nsICrashReporter);
  do_check_neq(cr, null);

  



  
  dump("INFO | test_crashreporter.js | Disable crashreporter.\n");
  cr.enabled = false;
  do_check_false(cr.enabled);

  try {
    let su = cr.serverURL;
    do_throw("Getting serverURL when disabled should have thrown!");
  }
  catch (ex) {
    do_check_eq(ex.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  try {
    let mdp = cr.minidumpPath;
    do_throw("Getting minidumpPath when disabled should have thrown!");
  }
  catch (ex) {
    do_check_eq(ex.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  try {
    cr.annotateCrashReport(null, null);
    do_throw("Calling annotateCrashReport() when disabled should have thrown!");
  }
  catch (ex) {
    do_check_eq(ex.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  try {
    cr.appendAppNotesToCrashReport(null);
    do_throw("Calling appendAppNotesToCrashReport() when disabled should have thrown!");
  }
  catch (ex) {
    do_check_eq(ex.result, Components.results.NS_ERROR_NOT_INITIALIZED);
  }

  



  dump("INFO | test_crashreporter.js | Re-enable crashreporter (in default state).\n");
  
  cr.enabled = true;
  do_check_true(cr.enabled);
  
  cr.enabled = true;
  do_check_true(cr.enabled);

  try {
    let su = cr.serverURL;
    do_throw("Getting serverURL when not set should have thrown!");
  }
  catch (ex) {
    do_check_eq(ex.result, Components.results.NS_ERROR_FAILURE);
  }

  
  var ios = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);

  
  var testspecs = ["http://example.com/submit",
                   "https://example.org/anothersubmit"];
  for (var i = 0; i < testspecs.length; ++i) {
    cr.serverURL = ios.newURI(testspecs[i], null, null);
    do_check_eq(cr.serverURL.spec, testspecs[i]);
  }

  
  try {
    cr.serverURL = ios.newURI("ftp://example.com/submit", null, null);
    do_throw("Setting serverURL to a non-http(s) URL should have thrown!");
  }
  catch (ex) {
    do_check_eq(ex.result, Components.results.NS_ERROR_INVALID_ARG);
  }

  
  
  
  do_check_neq(cr.minidumpPath.path, "");
  var cwd = do_get_cwd();
  cr.minidumpPath = cwd;
  do_check_eq(cr.minidumpPath.path, cwd.path);

  try {
    cr.annotateCrashReport("=", "");
    do_throw("Calling annotateCrashReport() with an '=' key should have thrown!");
  }
  catch (ex) {
    do_check_eq(ex.result, Components.results.NS_ERROR_INVALID_ARG);
  }
  try {
    cr.annotateCrashReport("\n", "");
    do_throw("Calling annotateCrashReport() with a '\\n' key should have thrown!");
  }
  catch (ex) {
    do_check_eq(ex.result, Components.results.NS_ERROR_INVALID_ARG);
  }
  try {
    cr.annotateCrashReport("", "\0");
    do_throw("Calling annotateCrashReport() with a '\\0' data should have thrown!");
  }
  catch (ex) {
    do_check_eq(ex.result, Components.results.NS_ERROR_INVALID_ARG);
  }
  cr.annotateCrashReport("testKey", "testData");

  try {
    cr.appendAppNotesToCrashReport("\0");
    do_throw("Calling appendAppNotesToCrashReport() with a '\\0' data should have thrown!");
  }
  catch (ex) {
    do_check_eq(ex.result, Components.results.NS_ERROR_INVALID_ARG);
  }
  cr.appendAppNotesToCrashReport("additional testData");

  
  cr.enabled = false;
  do_check_false(cr.enabled);
  
  cr.enabled = false;
  do_check_false(cr.enabled);

  



  
  dump("INFO | test_crashreporter.js | Reset crashreporter to its initial state.\n");
  
  cr.enabled = true;
  do_check_true(cr.enabled);
  cr.minidumpPath = cwd;
  do_check_eq(cr.minidumpPath.path, cwd.path);
}
