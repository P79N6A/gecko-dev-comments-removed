Cu.import("resource://gre/modules/Services.jsm");

function run_test() {
  do_test_pending();

  function StreamListener() {}

  StreamListener.prototype = {
    QueryInterface: function(aIID) {
      if (aIID.equals(Components.interfaces.nsIStreamListener) ||
          aIID.equals(Components.interfaces.nsIRequestObserver) ||
          aIID.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_NOINTERFACE;
    },

    onStartRequest: function(aRequest, aContext) {},

    onStopRequest: function(aRequest, aContext, aStatusCode) {
      
      do_check_eq(aStatusCode, Components.results.NS_ERROR_FILE_NOT_FOUND);
      do_test_finished();
    },

    onDataAvailable: function(aRequest, aContext, aStream, aOffset, aCount) {
      do_throw("The channel must not call onDataAvailable().");
    }
  };

  let listener = new StreamListener();
  let ios = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);

  
  let file = do_get_file("_NOT_EXIST_.txt", true);
  do_check_false(file.exists());

  let channel = ios.newChannelFromURI2(ios.newFileURI(file),
                                       null,      
                                       Services.scriptSecurityManager.getSystemPrincipal(),
                                       null,      
                                       Ci.nsILoadInfo.SEC_NORMAL,
                                       Ci.nsIContentPolicy.TYPE_OTHER);
  channel.asyncOpen(listener, null);
}
