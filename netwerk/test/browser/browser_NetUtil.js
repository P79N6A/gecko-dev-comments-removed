




Components.utils.import("resource://gre/modules/NetUtil.jsm");

function test() {
  waitForExplicitFinish();

  
  
  Components.utils.import("resource://testing-common/httpd.js", {});

  nextTest();
}

function nextTest() {
  if (tests.length)
    executeSoon(tests.shift());
  else
    executeSoon(finish);
}

var tests = [
  test_asyncFetchBadCert,
];

function test_asyncFetchBadCert() {
  
  NetUtil.asyncFetch("https://untrusted.example.com", function (aInputStream, aStatusCode, aRequest) {
    ok(!Components.isSuccessCode(aStatusCode), "request failed");
    ok(aRequest instanceof Ci.nsIHttpChannel, "request is an nsIHttpChannel");

    
    let channel = NetUtil.newChannel("https://untrusted.example.com");
    channel.notificationCallbacks = {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIProgressEventSink,
                                             Ci.nsIInterfaceRequestor]),
      getInterface: function (aIID) this.QueryInterface(aIID),
      onProgress: function () {},
      onStatus: function () {}
    };
    NetUtil.asyncFetch(channel, function (aInputStream, aStatusCode, aRequest) {
      ok(!Components.isSuccessCode(aStatusCode), "request failed");
      ok(aRequest instanceof Ci.nsIHttpChannel, "request is an nsIHttpChannel");

      
      NetUtil.asyncFetch("https://example.com", function (aInputStream, aStatusCode, aRequest) {
        info("aStatusCode for valid request: " + aStatusCode);
        ok(Components.isSuccessCode(aStatusCode), "request succeeded");
        ok(aRequest instanceof Ci.nsIHttpChannel, "request is an nsIHttpChannel");
        ok(aRequest.requestSucceeded, "HTTP request succeeded");
  
        nextTest();
      });
    });

  });
}

function WindowListener(aURL, aCallback) {
  this.callback = aCallback;
  this.url = aURL;
}
WindowListener.prototype = {
  onOpenWindow: function(aXULWindow) {
    var domwindow = aXULWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIDOMWindow);
    var self = this;
    domwindow.addEventListener("load", function() {
      domwindow.removeEventListener("load", arguments.callee, false);

      if (domwindow.document.location.href != self.url)
        return;

      
      executeSoon(function() {
        self.callback(domwindow);
      });
    }, false);
  },
  onCloseWindow: function(aXULWindow) {},
  onWindowTitleChange: function(aXULWindow, aNewTitle) {}
}
