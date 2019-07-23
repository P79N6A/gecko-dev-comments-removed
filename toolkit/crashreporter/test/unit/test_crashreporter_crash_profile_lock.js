function run_test()
{
  if (!("@mozilla.org/toolkit/crash-reporter;1" in Components.classes)) {
    dump("INFO | test_crashreporter.js | Can't test crashreporter in a non-libxul build.\n");
    return;
  }

  
  
  
  do_crash(function() {
             let env = Components.classes["@mozilla.org/process/environment;1"]
               .getService(Components.interfaces.nsIEnvironment);
             
             let profd = env.get("XPCSHELL_TEST_PROFILE_DIR");
             let dir = Components.classes["@mozilla.org/file/local;1"]
               .createInstance(Components.interfaces.nsILocalFile);
             dir.initWithPath(profd);
             let mycrasher = Components.classes["@mozilla.org/testcrasher;1"].createInstance(Components.interfaces.nsITestCrasher);
             let lock = mycrasher.lockDir(dir);
             
           },
           function(mdump, extra) {
             
             do_check_true(true);
           });
}
