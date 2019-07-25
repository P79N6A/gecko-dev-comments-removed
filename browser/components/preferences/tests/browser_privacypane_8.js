



function test() {
  let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
               getService(Ci.mozIJSSubScriptLoader);
  let rootDir = getRootDirectory(gTestPath);
  let jar = getJar(rootDir);
  if (jar) {
    let tmpdir = extractJarToTmp(jar);
    rootDir = "file://" + tmpdir.path + '/';
  }
  loader.loadSubScript(rootDir + "privacypane_tests.js", this);

  run_test_subset([
    
    test_historymode_retention("remember", undefined),

    
    test_custom_retention("acceptCookies", "remember"),

    
    test_historymode_retention("dontremember", "custom"),

    
    test_historymode_retention("remember", "custom"),

    
    test_historymode_retention("remember", "remember"),

    
    reset_preferences
  ]);
}
