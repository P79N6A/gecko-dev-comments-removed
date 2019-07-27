

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm");

const path = "data/test_bug589292.zip";

function run_test() {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  var spec = "jar:" + ios.newFileURI(do_get_file(path)).spec + "!/foo.txt";
  var channel = ios.newChannel2(spec,
                                null,
                                null,
                                null,      
                                Services.scriptSecurityManager.getSystemPrincipal(),
                                null,      
                                Ci.nsILoadInfo.SEC_NORMAL,
                                Ci.nsIContentPolicy.TYPE_OTHER);
  instr = channel.open();
  var val;
  try {
    val = channel.contentDisposition;
    do_check_true(false, "The channel has content disposition?!");
  } catch (e) {
    
    
    do_check_true(true, "How are you reading this?!");
  }
}
