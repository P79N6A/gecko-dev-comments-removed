


let tempScope = {};
Cu.import("resource://gre/modules/devtools/Loader.jsm", tempScope);
Cu.import("resource://gre/modules/devtools/Console.jsm", tempScope);
const require = tempScope.devtools.require;
const console = tempScope.console;
tempScope = null;
const PATH = "browser/toolkit/devtools/server/tests/browser/";
const MAIN_DOMAIN = "http://test1.example.org/" + PATH;
const ALT_DOMAIN = "http://sectest1.example.org/" + PATH;
const ALT_DOMAIN_SECURED = "https://sectest1.example.org:443/" + PATH;




function addTab(aURL, aCallback) {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  content.location = aURL;

  let tab = gBrowser.selectedTab;
  let browser = gBrowser.getBrowserForTab(tab);

  function onTabLoad(event) {
    if (event.originalTarget.location.href != aURL) {
      return;
    }
    browser.removeEventListener("load", onTabLoad, true);
    aCallback(browser.contentDocument);
  }

  browser.addEventListener("load", onTabLoad, true);
}





function forceCollections() {
  Cu.forceGC();
  Cu.forceCC();
  Cu.forceShrinkingGC();
}

registerCleanupFunction(function tearDown() {
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});
