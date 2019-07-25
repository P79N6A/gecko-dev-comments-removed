




Components.utils.import("resource://gre/modules/NetUtil.jsm");

function test() {
  waitForExplicitFinish();

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

var gCertErrorDialogShown = 0;




function test_asyncFetchBadCert() {
  let listener = new WindowListener("chrome://pippki/content/certerror.xul", function (domwindow) {
    gCertErrorDialogShown++;

    
    domwindow.document.documentElement.cancelDialog();
  });

  Services.wm.addListener(listener);

  
  NetUtil.asyncFetch("https://untrusted.example.com", function (aInputStream, aStatusCode, aRequest) {
    ok(!Components.isSuccessCode(aStatusCode), "request failed");
    ok(aRequest instanceof Ci.nsIHttpChannel, "request is an nsIHttpChannel");

    is(gCertErrorDialogShown, 0, "cert error dialog was not shown");

    
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

      is(gCertErrorDialogShown, 0, "cert error dialog was not shown");

      
      NetUtil.asyncFetch("https://example.com", function (aInputStream, aStatusCode, aRequest) {
        ok(Components.isSuccessCode(aStatusCode), "request succeeded");
        ok(aRequest instanceof Ci.nsIHttpChannel, "request is an nsIHttpChannel");
        ok(aRequest.requestSucceeded, "HTTP request succeeded");
  
        is(gCertErrorDialogShown, 0, "cert error dialog was not shown");

        Services.wm.removeListener(listener);
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
