



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm");


var ios = Cc["@mozilla.org/network/io-service;1"].
          getService(Ci.nsIIOService);

function run_test() {
  var file = do_get_file("data/test_bug658093.zip");
  var spec = "jar:" + ios.newFileURI(file).spec + "!/0000";
  var channel = ios.newChannel2(spec,
                                null,
                                null,
                                null,      
                                Services.scriptSecurityManager.getSystemPrincipal(),
                                null,      
                                Ci.nsILoadInfo.SEC_NORMAL,
                                Ci.nsIContentPolicy.TYPE_OTHER);
  var failed = false;
  try {
    var stream = channel.open();
  } catch (e) {
    failed = true;
  }
  do_check_true(failed);
}
