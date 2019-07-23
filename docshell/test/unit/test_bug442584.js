var prefetch = Cc["@mozilla.org/prefetch-service;1"].
               getService(Ci.nsIPrefetchService);
var ios = Cc["@mozilla.org/network/io-service;1"].
          getService(Ci.nsIIOService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch);

function run_test() {
  
  prefs.setBoolPref("network.prefetch-next", true);
  for (var i = 0; i < 5; i++) {
    var uri = ios.newURI("http://localhost/" + i, null, null);
    prefetch.prefetchURI(uri, uri, null, true);
  }

  
  var queue = prefetch.enumerateQueue(true, false);
  do_check_true(queue.hasMoreElements());

  
  prefs.setBoolPref("network.prefetch-next", false);
  queue = prefetch.enumerateQueue(true, false);
  do_check_false(queue.hasMoreElements());

  
  prefs.setBoolPref("network.prefetch-next", true);
  for (var i = 0; i < 5; i++) {
    var uri = ios.newURI("http://localhost/" + i, null, null);
    prefetch.prefetchURI(uri, uri, null, true);
  }
  queue = prefetch.enumerateQueue(true, false);
  do_check_true(queue.hasMoreElements());
}

