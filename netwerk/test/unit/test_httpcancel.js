





Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");

var ios = Components.classes["@mozilla.org/network/io-service;1"]
                    .getService(Components.interfaces.nsIIOService);
var observer = {
  QueryInterface: function eventsink_qi(iid) {
    if (iid.equals(Components.interfaces.nsISupports) ||
        iid.equals(Components.interfaces.nsIObserver))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  observe: function(subject, topic, data) {
    subject = subject.QueryInterface(Components.interfaces.nsIRequest);
    subject.cancel(Components.results.NS_BINDING_ABORTED);

    
    try {
      subject.QueryInterface(Components.interfaces.nsIHttpChannel);
      currentReferrer = subject.getRequestHeader("Referer");
      do_check_eq(currentReferrer, "http://site1.com/");
      var uri = ios.newURI("http://site2.com", null, null);
      subject.referrer = uri;
    } catch (ex) {
      do_throw("Exception: " + ex);
    }

    var obs = Components.classes["@mozilla.org/observer-service;1"].getService();
    obs = obs.QueryInterface(Components.interfaces.nsIObserverService);
    obs.removeObserver(observer, "http-on-modify-request");
  }
};

var listener = {
  onStartRequest: function test_onStartR(request, ctx) {
    do_check_eq(request.status, Components.results.NS_BINDING_ABORTED);

    
    try {
      request.QueryInterface(Components.interfaces.nsIHttpChannel);
      currentReferrer = request.getRequestHeader("Referer");
      do_check_eq(currentReferrer, "http://site2.com/");
      var uri = ios.newURI("http://site3.com/", null, null);

      
      var env = Components.classes["@mozilla.org/process/environment;1"].
                  getService(Components.interfaces.nsIEnvironment);
      env.set("NECKO_ERRORS_ARE_FATAL", "0");
      
      try {
        request.referrer = uri;
        do_throw("Error should have been thrown before getting here");
      } catch (ex) { } 
    } catch (ex) {
      do_throw("Exception: " + ex);
    }
  },

  onDataAvailable: function test_ODA() {
    do_throw("Should not get any data!");
  },

  onStopRequest: function test_onStopR(request, ctx, status) {
    httpserv.stop(do_test_finished);
  }
};

function makeChan(url) {
  var chan = ios.newChannel2(url,
                             null,
                             null,
                             null,      
                             Services.scriptSecurityManager.getSystemPrincipal(),
                             null,      
                             Ci.nsILoadInfo.SEC_NORMAL,
                             Ci.nsIContentPolicy.TYPE_OTHER)
                .QueryInterface(Components.interfaces.nsIHttpChannel);

  
  var uri = ios.newURI("http://site1.com", null, null);
  chan.referrer = uri;

  return chan;
}

var httpserv = null;

function execute_test() {
  var chan = makeChan("http://localhost:" +
                      httpserv.identity.primaryPort + "/failtest");

  var obs = Components.classes["@mozilla.org/observer-service;1"].getService();
  obs = obs.QueryInterface(Components.interfaces.nsIObserverService);
  obs.addObserver(observer, "http-on-modify-request", false);

  chan.asyncOpen(listener, null);
}

function run_test() {
  httpserv = new HttpServer();
  httpserv.registerPathHandler("/failtest", failtest);
  httpserv.start(-1);

  execute_test();

  do_test_pending();
}




function failtest(metadata, response) {
  do_throw("This should not be reached");
}
