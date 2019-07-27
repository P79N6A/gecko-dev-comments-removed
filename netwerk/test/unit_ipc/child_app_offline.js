
function inChildProcess() {
  return Cc["@mozilla.org/xre/app-info;1"]
           .getService(Ci.nsIXULRuntime)
           .processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
}

function makeChan(url, appId, inBrowser) {
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var chan = ios.newChannel(url, null, null).QueryInterface(Ci.nsIHttpChannel);
  chan.notificationCallbacks = {
    appId: appId,
    isInBrowserElement: inBrowser,
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsILoadContext))
        return this;
      throw Cr.NS_ERROR_NO_INTERFACE;
    },
    getInterface: function(iid) { return this.QueryInterface(iid); }
  };
  return chan;
}


function run_test() {
  do_test_pending();
  var chan = makeChan("http://localhost:12345/first", 14, false);
  chan.asyncOpen(new ChannelListener(checkResponse, "response0"), null);
}


function test1() {
  do_test_pending();
  var chan = makeChan("http://localhost:12345/first", 14, false);
  chan.asyncOpen(new ChannelListener(checkResponse, "response0"), null);
}


function test2() {
  do_test_pending();
  var chan = makeChan("http://localhost:12345/second", 14, false);
  chan.asyncOpen(new ChannelListener(checkResponse, "", CL_EXPECT_FAILURE), null);
}


function test3() {
  do_test_pending();
  var chan = makeChan("http://localhost:12345/second", 14, false);
  chan.asyncOpen(new ChannelListener(checkResponse, "response3"), null);
}

function checkResponse(req, buffer, expected) {
  do_check_eq(buffer, expected);
  do_test_finished();
}
