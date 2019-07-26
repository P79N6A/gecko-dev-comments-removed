









var count = 0;
const URIS = ["about:config",
              "about:plugins",
              "about:buildconfig",
              "data:text/html,<title>OK</title>"];

function test() {
  waitForExplicitFinish();
  URIS.forEach(addTab);
}

function addTab(aURI, aIndex) {
  var tab = gBrowser.addTab(aURI);
  if (aIndex == 0)
    gBrowser.removeTab(gBrowser.tabs[0]);

  tab.linkedBrowser.addEventListener("load", function (event) {
    event.currentTarget.removeEventListener("load", arguments.callee, true);
    if (++count == URIS.length)
      executeSoon(doTabsTest);
  }, true);
}

function doTabsTest() {
  is(gBrowser.tabs.length, URIS.length, "Correctly opened all expected tabs");

  
  gBrowser.tabContainer.addEventListener("TabClose", function (event) {
    event.currentTarget.removeEventListener("TabClose", arguments.callee, true);
    var closedTab = event.originalTarget;
    var scheme = closedTab.linkedBrowser.currentURI.scheme;
    Array.slice(gBrowser.tabs).forEach(function (aTab) {
      if (aTab != closedTab && aTab.linkedBrowser.currentURI.scheme == scheme)
        gBrowser.removeTab(aTab);
    });
  }, true);

  gBrowser.removeTab(gBrowser.tabs[0]);
  is(gBrowser.tabs.length, 1, "Related tabs are not closed unexpectedly");

  gBrowser.addTab("about:blank");
  gBrowser.removeTab(gBrowser.tabs[0]);
  finish();
}
