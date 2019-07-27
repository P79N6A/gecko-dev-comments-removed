const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm");



function run_test() {
  
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
  var file = do_get_file("data/test_bug370103.jar");
  var url = ioService.newFileURI(file).spec;
  url = "jar:" + url + "!/test_bug370103";

  
  var channel = ioService.newChannel2(url,
                                      null,
                                      null,
                                      null,      
                                      Services.scriptSecurityManager.getSystemPrincipal(),
                                      null,      
                                      Ci.nsILoadInfo.SEC_NORMAL,
                                      Ci.nsIContentPolicy.TYPE_OTHER);

  var exception = false;
  try {
    channel.asyncOpen(null, null);
  }
  catch(e) {
    exception = true;
  }

  do_check_true(exception); 
}
