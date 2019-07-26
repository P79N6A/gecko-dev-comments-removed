



let tempScope = {};
Components.utils.import("resource:///modules/devtools/Target.jsm", tempScope);
let TargetFactory = tempScope.TargetFactory;
Components.utils.import("resource://gre/modules/devtools/Console.jsm", tempScope);
let console = tempScope.console;
Components.utils.import("resource://gre/modules/commonjs/sdk/core/promise.js", tempScope);
let Promise = tempScope.Promise;




function addTab(aURL, aCallback)
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  if (aURL != null) {
    content.location = aURL;
  }

  let deferred = Promise.defer();

  let tab = gBrowser.selectedTab;
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let browser = gBrowser.getBrowserForTab(tab);

  function onTabLoad() {
    browser.removeEventListener("load", onTabLoad, true);

    if (aCallback != null) {
      aCallback(browser, tab, browser.contentDocument);
    }

    deferred.resolve({ browser: browser, tab: tab, target: target });
  }

  browser.addEventListener("load", onTabLoad, true);
  return deferred.promise;
}

registerCleanupFunction(function tearDown() {
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});
