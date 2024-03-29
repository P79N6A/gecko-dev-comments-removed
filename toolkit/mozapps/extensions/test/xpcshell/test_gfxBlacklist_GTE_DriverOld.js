




const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;





Cu.import("resource://testing-common/httpd.js");

var gTestserver = new HttpServer();
gTestserver.start(-1);
gPort = gTestserver.identity.primaryPort;
mapFile("/data/test_gfxBlacklist.xml", gTestserver);

function get_platform() {
  var xulRuntime = Cc["@mozilla.org/xre/app-info;1"]
                             .getService(Ci.nsIXULRuntime);
  return xulRuntime.OS;
}

function load_blocklist(file) {
  Services.prefs.setCharPref("extensions.blocklist.url", "http://localhost:" +
                             gPort + "/data/" + file);
  var blocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                  getService(Ci.nsITimerCallback);
  blocklist.notify(null);
}


function run_test() {
  var gfxInfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfo);

  
  if (!(gfxInfo instanceof Ci.nsIGfxInfoDebug)) {
    do_test_finished();
    return;
  }

  gfxInfo.QueryInterface(Ci.nsIGfxInfoDebug);

  
  switch (get_platform()) {
    case "WINNT":
      gfxInfo.spoofVendorID("0xabab");
      gfxInfo.spoofDeviceID("0x1234");
      gfxInfo.spoofDriverVersion("8.52.322.2201");
      
      gfxInfo.spoofOSVersion(0x60001);
      break;
    case "Linux":
      
      do_test_finished();
      return;
    case "Darwin":
      
      do_test_finished();
      return;
    case "Android":
      gfxInfo.spoofVendorID("abab");
      gfxInfo.spoofDeviceID("ghjk");
      gfxInfo.spoofDriverVersion("6");
      break;
  }

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "3", "8");
  startupManager();

  do_test_pending();

  function checkBlacklist()
  {
    var status = gfxInfo.getFeatureStatus(Ci.nsIGfxInfo.FEATURE_DIRECT2D);
    do_check_eq(status, Ci.nsIGfxInfo.FEATURE_STATUS_OK);

    
    status = gfxInfo.getFeatureStatus(Ci.nsIGfxInfo.FEATURE_DIRECT3D_9_LAYERS);
    do_check_eq(status, Ci.nsIGfxInfo.FEATURE_STATUS_OK);

    gTestserver.stop(do_test_finished);
  }

  Services.obs.addObserver(function(aSubject, aTopic, aData) {
    
    
    do_execute_soon(checkBlacklist);
  }, "blocklist-data-gfxItems", false);

  load_blocklist("test_gfxBlacklist.xml");
}
