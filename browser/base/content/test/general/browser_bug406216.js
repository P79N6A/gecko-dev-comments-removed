









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

  tab.linkedBrowser.addEventListener("load", function (event) {
    event.currentTarget.removeEventListener("load", arguments.callee, true);
    if (++count == URIS.length)
      executeSoon(doTabsTest);
  }, true);
}

function doTabsTest() {
  is(gBrowser.tabs.length - 1, URIS.length, "Correctly opened all expected tabs");

  
  gBrowser.tabContainer.addEventListener("TabClose", function (event) {
    event.currentTarget.removeEventListener("TabClose", arguments.callee, true);
    var closedTab = event.originalTarget;
    var scheme = closedTab.linkedBrowser.currentURI.scheme;
    Array.slice(gBrowser.tabs).forEach(function (aTab, aIndex) {
      if (aIndex != 0 && aTab != closedTab && aTab.linkedBrowser.currentURI.scheme == scheme)
        gBrowser.removeTab(aTab);
    });
  }, true);

  gBrowser.removeTab(gBrowser.tabs[1]);
  is(gBrowser.tabs.length - 1, 1, "Related tabs are not closed unexpectedly");

  gBrowser.removeTab(gBrowser.tabs[1]);
  finish();
}
