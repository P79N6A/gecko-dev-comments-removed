





var gExpectedFile = null;
var gCacheFlushed = false;

var CacheFlushObserver = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic != "flush-cache-entry")
      return;

    do_check_true(gExpectedFile != null);
    do_check_true(aSubject instanceof AM_Ci.nsIFile);
    do_check_eq(aSubject.path, gExpectedFile.path);
    gCacheFlushed = true;
    gExpectedFile = null;
  }
};

function run_test() {
  
  if (Services.prefs.getBoolPref("extensions.alwaysUnpack"))
    return;

  do_test_pending();
  Services.obs.addObserver(CacheFlushObserver, "flush-cache-entry", false);
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "2");

  startupManager();

  run_test_1();
}


function run_test_1() {
  AddonManager.getInstallForFile(do_get_addon("test_cacheflush1"), function(aInstall) {
    completeAllInstalls([aInstall], function() {
      
      gExpectedFile = gProfD.clone();
      gExpectedFile.append("extensions");
      gExpectedFile.append("staged");
      gExpectedFile.append("addon1@tests.mozilla.org.xpi");
      aInstall.cancel();

      do_check_true(gCacheFlushed);
      gCacheFlushed = false;

      run_test_2();
    });
  });
}


function run_test_2() {
  installAllFiles([do_get_addon("test_cacheflush1")], function() {
    
    gExpectedFile = gProfD.clone();
    gExpectedFile.append("extensions");
    gExpectedFile.append("staged");
    gExpectedFile.append("addon1@tests.mozilla.org.xpi");
    restartManager();
    do_check_true(gCacheFlushed);
    gCacheFlushed = false;

    AddonManager.getAddonByID("addon1@tests.mozilla.org", function(a1) {
      
      gExpectedFile = gProfD.clone();
      gExpectedFile.append("extensions");
      gExpectedFile.append("addon1@tests.mozilla.org.xpi");

      a1.uninstall();
      do_check_false(gCacheFlushed);
      restartManager();

      run_test_3();
    });
  });
}


function run_test_3() {
  AddonManager.getInstallForFile(do_get_addon("test_cacheflush2"), function(aInstall) {
    aInstall.addListener({
      onInstallStarted: function(aInstall) {
        
        gExpectedFile = gProfD.clone();
        gExpectedFile.append("extensions");
        gExpectedFile.append("staged");
        gExpectedFile.append("addon2@tests.mozilla.org.xpi");
      },

      onInstallEnded: function(aInstall) {
        do_check_true(gCacheFlushed);
        gCacheFlushed = false;

        run_test_4();
      }
    });

    aInstall.install();
  });
}


function run_test_4() {
  AddonManager.getAddonByID("addon2@tests.mozilla.org", function(a2) {
    
    gExpectedFile = gProfD.clone();
    gExpectedFile.append("extensions");
    gExpectedFile.append("addon2@tests.mozilla.org.xpi");

    a2.uninstall();
    do_check_true(gCacheFlushed);
    gCacheFlushed = false;

    do_test_finished();
  });
}
