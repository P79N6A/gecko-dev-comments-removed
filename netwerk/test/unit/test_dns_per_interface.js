var dns = Cc["@mozilla.org/network/dns-service;1"].getService(Ci.nsIDNSService);









var netInterface1 = "interface1";
var netInterface2 = "interface2";




var hostname = "thisshouldnotexist.mozilla.com";


var requestWithInterfaceCanceled;
var requestWithoutInterfaceNotCanceled;
var requestWithInterfaceNotCanceled;

var listener = {
  onLookupComplete: function(inRequest, inRecord, inStatus) {
    
    
    
    if ((inRequest == requestWithoutInterfaceNotCanceled) ||
        (inRequest == requestWithInterfaceNotCanceled)) {
      
      do_check_neq(inStatus, Cr.NS_ERROR_ABORT);

      do_test_finished();
    } else if (inRequest == requestWithInterfaceCanceled) {
      
      
      do_test_finished();
    }
  },
  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIDNSListener) ||
        aIID.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

function run_test() {
  var threadManager = Cc["@mozilla.org/thread-manager;1"]
                        .getService(Ci.nsIThreadManager);
  var mainThread = threadManager.currentThread;

  var flags = Ci.nsIDNSService.RESOLVE_BYPASS_CACHE;

  
  requestWithInterfaceCanceled = dns.asyncResolveExtended(hostname, flags,
                                                          netInterface1,
                                                          listener,
                                                          mainThread);
  requestWithInterfaceCanceled.cancel(Cr.NS_ERROR_ABORT);

  
  
  requestWithoutInterfaceNotCanceled = dns.asyncResolve(hostname, flags,
                                                        listener, mainThread);

  
  requestWithInterfaceNotCanceled = dns.asyncResolveExtended(hostname, flags,
                                                             netInterface2,
                                                             listener,
                                                             mainThread);
  
  
  do_test_pending();
  do_test_pending();
  do_test_pending();
}
