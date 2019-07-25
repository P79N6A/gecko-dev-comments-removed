


do_load_httpd_js();

const MIN_TIME_DIFFERENCE = 3000;
const RESUME_DELAY = 5000;

var listener = {
  _lastEvent: 0,
  _gotData: false,

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIStreamListener) ||
        iid.equals(Components.interfaces.nsIRequestObserver) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  onStartRequest: function(request, ctx) {
    this._lastEvent = Date.now();
    request.QueryInterface(Ci.nsIRequest);

    
    
    request.suspend();
    do_timeout(RESUME_DELAY, function() request.resume());
  },

  onDataAvailable: function(request, context, stream, offset, count) {
    do_check_true(Date.now() - this._lastEvent >= MIN_TIME_DIFFERENCE);
    read_stream(stream, count);

    
    request.suspend();
    request.resume();

    this._gotData = true;
  },

  onStopRequest: function(request, ctx, status) {
    do_check_true(this._gotData);
    httpserv.stop(do_test_finished);
  }
};

function makeChan(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var chan = ios.newChannel(url, null, null).QueryInterface(Ci.nsIHttpChannel);
  return chan;
}

var httpserv = null;

function run_test() {
  httpserv = new nsHttpServer();
  httpserv.registerPathHandler("/woo", data);
  httpserv.start(4444);

  var chan = makeChan("http://localhost:4444/woo");
  chan.QueryInterface(Ci.nsIRequest);
  chan.asyncOpen(listener, null);

  do_test_pending();
}

function data(metadata, response) {
  let httpbody = "0123456789";
  response.setHeader("Content-Type", "text/plain", false);
  response.bodyOutputStream.write(httpbody, httpbody.length);
}
