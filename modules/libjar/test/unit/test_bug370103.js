var Cc = Components.classes;
var Ci = Components.interfaces;



function run_test() {
  
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
  var file = do_get_file("modules/libjar/test/unit/data/test_bug370103.jar");  
  var url = ioService.newFileURI(file).spec;
  url = "jar:" + url + "!/test_bug370103";

  
  var channel = ioService.newChannel(url, null, null);

  var exception = false;
  try {
    channel.asyncOpen(null, null);
  }
  catch(e) {
    exception = true;
  }

  do_check_true(exception); 
}
