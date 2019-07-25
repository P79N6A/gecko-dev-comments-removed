




function test() {
  waitForExplicitFinish();

  let tab = gBrowser.addTab("http://example.com");
  let tabBrowser = tab.linkedBrowser;

  tabBrowser.addEventListener("load", function loadListener(aEvent) {
    tabBrowser.removeEventListener("load", loadListener, true);

    let contentWindow = tab.linkedBrowser.contentWindow;
    gBrowser.removeTab(tab);

    SimpleTest.executeSoon(function() {
      ok(Components.utils.isDeadWrapper(contentWindow),
         'Window should be dead.');
      finish();
    });
  }, true);
}
