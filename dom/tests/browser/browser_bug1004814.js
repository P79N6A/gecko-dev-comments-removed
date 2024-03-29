




const TEST_URI = "http://example.com/browser/dom/tests/browser/test_bug1004814.html";

function test() {
  waitForExplicitFinish();

  ConsoleObserver.init();

  var tab = gBrowser.addTab(TEST_URI);
  gBrowser.selectedTab = tab;

  registerCleanupFunction(function () {
    gBrowser.removeTab(tab);
  });
}

var ConsoleObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  init: function() {
    Services.obs.addObserver(this, "console-api-log-event", false);
  },

  destroy: function() {
    Services.obs.removeObserver(this, "console-api-log-event");
  },

  observe: function(aSubject, aTopic, aData) {
    var obj = aSubject.wrappedJSObject;
    if (obj.arguments.length != 1 || obj.arguments[0] != 'bug1004814' ||
        obj.level != 'timeEnd') {
      return;
    }

    ok("timer" in obj, "ConsoleEvent contains 'timer' property");
    ok("duration" in obj.timer, "ConsoleEvent.timer contains 'duration' property");
    ok(obj.timer.duration > 0, "ConsoleEvent.timer.duration > 0: " + obj.timer.duration + " ~ 200ms");

    this.destroy();
    finish();
  }
};
