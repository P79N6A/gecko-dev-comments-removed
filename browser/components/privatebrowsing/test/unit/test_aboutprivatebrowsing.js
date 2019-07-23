







































function is_about_privatebrowsing_available() {
  try {
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
    var channel = ios.newChannel("about:privatebrowsing", null, null);
    var input = channel.open();
    var sinput = Cc["@mozilla.org/scriptableinputstream;1"].
                 createInstance(Ci.nsIScriptableInputStream);
    sinput.init(input);
    while (true)
      if (!sinput.read(1024).length)
        break;
    sinput.close();
    input.close();
    return true;
  } catch (ex if ("result" in ex && ex.result == Cr.NS_ERROR_MALFORMED_URI)) { 
  }

  return false;
}

function run_test_on_service() {
  
  var pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
           getService(Ci.nsIPrivateBrowsingService);

  
  do_check_true(is_about_privatebrowsing_available());

  
  pb.privateBrowsingEnabled = true;

  
  do_check_true(is_about_privatebrowsing_available());

  
  pb.privateBrowsingEnabled = false;

  
  do_check_true(is_about_privatebrowsing_available());
}


function run_test() {
  run_test_on_all_services();
}
