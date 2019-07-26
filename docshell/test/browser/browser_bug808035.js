












function test() {
  const NEW_URI = "http://test1.example.org/";
  const REQUESTED_URI = "javascript:void(location.replace('" + NEW_URI +
                        "'))";

  waitForExplicitFinish();

  let tab = gBrowser.addTab(REQUESTED_URI);
  let browser = tab.linkedBrowser;

  browser.addEventListener('load', function(aEvent) {
    browser.removeEventListener('load', arguments.callee, true);

    is(browser.contentWindow.location.href, NEW_URI, "The URI is OK.");
    is(browser.contentWindow.history.length, 1, "There exists a SH entry.");

    gBrowser.removeTab(tab);
    finish();
  }, true);
}
