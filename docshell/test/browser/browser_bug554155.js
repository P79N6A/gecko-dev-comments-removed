function test() {
  waitForExplicitFinish();

  let tab = gBrowser.addTab("http://example.com");

  tab.linkedBrowser.addEventListener('load', function() {
    let numLocationChanges = 0;

    let listener = {
      onStateChange:    function() {},
      onProgressChange: function() {},
      onStatusChange:   function() {},
      onSecurityChange: function() {},
      onLocationChange: function() {
        numLocationChanges++;
      }
    };

    gBrowser.addTabsProgressListener(listener, Components.interfaces.nsIWebProgress.NOTIFY_ALL);

    
    
    tab.linkedBrowser.contentWindow.history.pushState(null, null, "foo");

    executeSoon(function() {
      is(numLocationChanges, 1,
         "pushState should cause exactly one LocationChange event.");
      finish();
    });

  }, true);
}
