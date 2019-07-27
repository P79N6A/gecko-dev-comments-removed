var dns = Cc["@mozilla.org/network/dns-service;1"].getService(Ci.nsIDNSService);

var hostname1 = "mozilla.org";
var hostname2 = "mozilla.com";

var requestList1Canceled1;
var requestList1Canceled2;
var requestList1NotCanceled;

var requestList2Canceled;
var requestList2NotCanceled;

var listener1 = {
  onLookupComplete: function(inRequest, inRecord, inStatus) {
    
    if (inRequest == requestList1Canceled1 ||
        inRequest == requestList1Canceled2) {
      
      do_check_eq(inStatus, Cr.NS_ERROR_ABORT);

      do_test_finished();
    } else if (inRequest == requestList1NotCanceled) {
      
      do_check_neq(inStatus, Cr.NS_ERROR_ABORT);

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

var listener2 = {
  onLookupComplete: function(inRequest, inRecord, inStatus) {
    
    if (inRequest == requestList2Canceled) {
      
      do_check_eq(inStatus, Cr.NS_ERROR_ABORT);

      do_test_finished();
    } else {
      
      do_check_neq(inStatus, Cr.NS_ERROR_ABORT);

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
  var threadManager = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
  var mainThread = threadManager.currentThread;

  var flags = Ci.nsIDNSService.RESOLVE_BYPASS_CACHE;

  
  requestList1Canceled1 = dns.asyncResolve(hostname2, flags, listener1, mainThread);
  dns.cancelAsyncResolve(hostname2, flags, listener1, Cr.NS_ERROR_ABORT);

  
  requestList1NotCanceled = dns.asyncResolve(hostname1, flags, listener1, mainThread);

  
  requestList1Canceled2 = dns.asyncResolve(hostname1, flags, listener1, mainThread);
  requestList1Canceled2.cancel(Cr.NS_ERROR_ABORT);

  
  requestList2NotCanceled = dns.asyncResolve(hostname1, flags, listener2, mainThread);

  
  requestList2Canceled = dns.asyncResolve(hostname2, flags, listener2, mainThread);
  requestList2Canceled.cancel(Cr.NS_ERROR_ABORT);

  do_test_pending();
  do_test_pending();
  do_test_pending();
  do_test_pending();
  do_test_pending();
}
