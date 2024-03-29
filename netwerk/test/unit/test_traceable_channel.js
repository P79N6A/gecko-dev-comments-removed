




Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");

var httpserver = new HttpServer();
httpserver.start(-1);
const PORT = httpserver.identity.primaryPort;

var pipe = null;
var streamSink = null;

var originalBody = "original http response body";
var gotOnStartRequest = false;

function TracingListener() {}

TracingListener.prototype = {
  onStartRequest: function(request, context) {
    dump("*** tracing listener onStartRequest\n");

    gotOnStartRequest = true;

    request.QueryInterface(Components.interfaces.nsIHttpChannelInternal);










    
    request.QueryInterface(Components.interfaces.nsITraceableChannel);
    try {
      var newListener = new TracingListener();
      newListener.listener = request.setNewListener(newListener);
    } catch(e) {
      dump("TracingListener.onStartRequest swallowing exception: " + e + "\n");
      return; 
    }
    do_throw("replaced channel's listener during onStartRequest.");
  },

  onStopRequest: function(request, context, statusCode) {
    dump("*** tracing listener onStopRequest\n");
    
    do_check_eq(gotOnStartRequest, true);

    try {
      var sin = Components.classes["@mozilla.org/scriptableinputstream;1"].
          createInstance(Ci.nsIScriptableInputStream);

      streamSink.close();
      var input = pipe.inputStream;
      sin.init(input);
      do_check_eq(sin.available(), originalBody.length);
    
      var result = sin.read(originalBody.length);
      do_check_eq(result, originalBody);
    
      input.close();
    } catch (e) {
      dump("TracingListener.onStopRequest swallowing exception: " + e + "\n");
    } finally {
      httpserver.stop(do_test_finished);
    }
  },

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIRequestObserver) ||
        iid.equals(Components.interfaces.nsISupports)
        )
      return this;
    throw Components.results.NS_NOINTERFACE;
  },

  listener: null
}


function HttpResponseExaminer() {}

HttpResponseExaminer.prototype = {
  register: function() {
    Cc["@mozilla.org/observer-service;1"].
      getService(Components.interfaces.nsIObserverService).
      addObserver(this, "http-on-examine-response", true);
    dump("Did HttpResponseExaminer.register\n");
  },

  
  observe: function(subject, topic, data) {
    dump("In HttpResponseExaminer.observe\n");
    try {
      subject.QueryInterface(Components.interfaces.nsITraceableChannel);
      
      var tee = Cc["@mozilla.org/network/stream-listener-tee;1"].
          createInstance(Ci.nsIStreamListenerTee);
      var newListener = new TracingListener();
      pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
      pipe.init(false, false, 0, 0xffffffff, null);
      streamSink = pipe.outputStream;
      
      var originalListener = subject.setNewListener(tee);
      tee.init(originalListener, streamSink, newListener);
    } catch(e) {
      do_throw("can't replace listener " + e);
    }
    dump("Did HttpResponseExaminer.observe\n");
  },

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIObserver) ||
        iid.equals(Components.interfaces.nsISupportsWeakReference) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  }
}

function test_handler(metadata, response) {
  response.setHeader("Content-Type", "text/html", false);
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.bodyOutputStream.write(originalBody, originalBody.length);
}

function make_channel(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
    getService(Ci.nsIIOService);
  return ios.newChannel2(url,
                         null,
                         null,
                         null,      
                         Services.scriptSecurityManager.getSystemPrincipal(),
                         null,      
                         Ci.nsILoadInfo.SEC_NORMAL,
                         Ci.nsIContentPolicy.TYPE_OTHER)
            .QueryInterface(Components.interfaces.nsIHttpChannel);
}


function channel_finished(request, input, ctx) {
  httpserver.stop(do_test_finished);
}

function run_test() {
  var observer = new HttpResponseExaminer();
  observer.register();

  httpserver.registerPathHandler("/testdir", test_handler);

  var channel = make_channel("http://localhost:" + PORT + "/testdir");
  channel.asyncOpen(new ChannelListener(channel_finished), null);
  do_test_pending();
}
