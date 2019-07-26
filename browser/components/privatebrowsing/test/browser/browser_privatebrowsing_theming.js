






var windowsToClose = [];
function testOnWindow(options, callback) {
  var win = OpenBrowserWindow(options);
  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);
    windowsToClose.push(win);
    executeSoon(function() callback(win));
  }, false);
}

registerCleanupFunction(function() {
  windowsToClose.forEach(function(win) {
    win.close();
  });
});

function test() {
  
  waitForExplicitFinish();

  ok(!document.documentElement.hasAttribute("privatebrowsingmode"),
    "privatebrowsingmode should not be present in normal mode");

  
  testOnWindow({private: true}, function(win) {
    is(win.document.documentElement.getAttribute("privatebrowsingmode"), "temporary",
      "privatebrowsingmode should be \"temporary\" inside the private browsing mode");

    finish();
  });
}
