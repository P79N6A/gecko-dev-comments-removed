










Components.utils.import("resource://testing-common/httpd.js");

var notification = "http-on-modify-request";

var httpServer = null;

var authCredentials = "guest:guest";
var authPath = "/authTest";
var authCredsURL = "http://" + authCredentials + "@localhost:8888" + authPath;
var authURL = "http://localhost:8888" + authPath;

function authHandler(metadata, response) {
  if (metadata.hasHeader("Test")) {
    
    var noAuthHeader = false;
    if (!metadata.hasHeader("Authorization")) {
      noAuthHeader = true;
    }
    do_check_true(noAuthHeader);
  } else {
    
    if (!metadata.hasHeader("Authorization")) {
      response.setStatusLine(metadata.httpVersion, 401, "Unauthorized");
      response.setHeader("WWW-Authenticate", 'Basic realm="secret"', false);
    }
  }
}

function RequestObserver() {
  this.register();
}

RequestObserver.prototype = {
  register: function() {
    do_print("Registering " + notification);
    Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService).
      addObserver(this, notification, true);
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIObserver) || iid.equals(Ci.nsISupportsWeakReference) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  observe: function(subject, topic, data) {
    if (topic == notification) {
      if (!(subject instanceof Ci.nsIHttpChannel)) {
        do_throw(notification + " observed a non-HTTP channel.");
      }
      try {
        let authHeader = subject.getRequestHeader("Authorization");
      } catch (e) {
        
        
        
        httpServer.stop(do_test_finished);
        do_throw("No authorization header found, aborting!");
      }
      
      subject.setRequestHeader("Authorization", null, false);
    }
  }
}

var listener = {
  onStartRequest: function test_onStartR(request, ctx) {},

  onDataAvailable: function test_ODA() {
    do_throw("Should not get any data!");
  },

  onStopRequest: function test_onStopR(request, ctx, status) {
    if (current_test < (tests.length - 1)) {
      current_test++;
      tests[current_test]();
    } else {
      do_test_pending();
      httpServer.stop(do_test_finished);
    }
    do_test_finished();
  }
};

function makeChan(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var chan = ios.newChannel(url, null, null).QueryInterface(Ci.nsIHttpChannel);
  return chan;
}

var tests = [startAuthHeaderTest, removeAuthHeaderTest];

var current_test = 0;

var requestObserver = null;

function run_test() {
  httpServer = new HttpServer();
  httpServer.registerPathHandler(authPath, authHandler);
  httpServer.start(8888);

  tests[0]();
}

function startAuthHeaderTest() {
  var chan = makeChan(authCredsURL);
  chan.asyncOpen(listener, null);

  do_test_pending();
}

function removeAuthHeaderTest() {
  
  
  requestObserver = new RequestObserver();
  var chan = makeChan(authURL);
  
  chan.setRequestHeader("Test", "1", false);
  chan.asyncOpen(listener, null);

  do_test_pending();
}
