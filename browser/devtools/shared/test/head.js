





































let tab;
let browser;

function addTab(aURL, aCallback)
{
  waitForExplicitFinish();

  function onTabLoad() {
    browser.removeEventListener("load", onTabLoad, true);
    aCallback();
  }

  gBrowser.selectedTab = gBrowser.addTab();
  content.location = aURL;

  tab = gBrowser.selectedTab;
  browser = gBrowser.getBrowserForTab(tab);

  browser.addEventListener("load", onTabLoad, true);
}

registerCleanupFunction(function tearDown() {
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }

  tab = undefined;
  browser = undefined;
});
