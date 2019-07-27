



Cu.import("resource://gre/modules/Services.jsm");



var url = "ws://localhost";
var chan;
var offlineStatus;

var listener = {
  onAcknowledge: function(aContext, aSize) {},
  onBinaryMessageAvailable: function(aContext, aMsg) {},
  onMessageAvailable: function(aContext, aMsg) {},
  onServerClose: function(aContext, aCode, aReason) {},
  onStart: function(aContext)
  {
    
    do_check_true(false);
  },
  onStop: function(aContext, aStatusCode)
  {
    do_check_neq(aStatusCode, Cr.NS_OK);
    Services.io.offline = offlineStatus;
    do_test_finished();
  }
};

function run_test() {
  offlineStatus = Services.io.offline;
  Services.io.offline = true;

  try {
    chan = Cc["@mozilla.org/network/protocol;1?name=ws"].
      createInstance(Components.interfaces.nsIWebSocketChannel);
    chan.initLoadInfo(null, 
                      Services.scriptSecurityManager.getSystemPrincipal(),
                      null, 
                      Ci.nsILoadInfo.SEC_NORMAL,
                      Ci.nsIContentPolicy.TYPE_WEBSOCKET);

    var uri = Services.io.newURI(url, null, null);
    chan.asyncOpen(uri, url, listener, null);
    do_test_pending();
  } catch (x) {
    dump("throwing " + x);
    do_throw(x);
  }
}
