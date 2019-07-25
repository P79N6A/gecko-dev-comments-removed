







Components.utils.import("resource://testing-common/httpd.js");

var gTestserver = null;

function get_platform() {
  var xulRuntime = Components.classes["@mozilla.org/xre/app-info;1"]
                             .getService(Components.interfaces.nsIXULRuntime);
  return xulRuntime.OS;
}

function load_blocklist(file) {
  Services.prefs.setCharPref("extensions.blocklist.url", "http://localhost:4444/data/" + file);
  var blocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                  getService(Ci.nsITimerCallback);
  blocklist.notify(null);
}


function run_test() {
  try {
    var gfxInfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfo);
  } catch (e) {
    do_test_finished();
    return;
  }

  
  if (!(gfxInfo instanceof Ci.nsIGfxInfoDebug)) {
    do_test_finished();
    return;
  }

  gfxInfo.QueryInterface(Ci.nsIGfxInfoDebug);

  
  gfxInfo.spoofDriverVersion("8.52.322.2202");
  gfxInfo.spoofVendorID("0xabcd");
  gfxInfo.spoofDeviceID("0x1234");

  
  switch (get_platform()) {
    case "WINNT":
      
      gfxInfo.spoofOSVersion(0x60002);
      break;
    case "Linux":
      
      do_test_finished();
      return;
    case "Darwin":
      
      gfxInfo.spoofOSVersion(0x1080);
      break;
    case "Android":
      
      
      do_test_finished();
      return;
  }

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "3", "8");
  startupManager();

  gTestserver = new HttpServer();
  gTestserver.registerDirectory("/data/", do_get_file("data"));
  gTestserver.start(4444);

  do_test_pending();

  function checkBlacklist()
  {
    if (get_platform() == "WINNT") {
      var status = gfxInfo.getFeatureStatus(Ci.nsIGfxInfo.FEATURE_DIRECT2D);
      do_check_eq(status, Ci.nsIGfxInfo.FEATURE_NO_INFO);
    } else if (get_platform() == "Darwin") {
      status = gfxInfo.getFeatureStatus(Ci.nsIGfxInfo.FEATURE_OPENGL_LAYERS);
      do_check_eq(status, Ci.nsIGfxInfo.FEATURE_NO_INFO);
    }

    gTestserver.stop(do_test_finished);
  }

  Services.obs.addObserver(function(aSubject, aTopic, aData) {
    
    
    do_execute_soon(checkBlacklist);
  }, "blocklist-data-gfxItems", false);

  load_blocklist("test_gfxBlacklist_OS.xml");
}
