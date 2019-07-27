







"use strict";

let test = asyncTest(function* () {
  const TEST_URI = "data:text/html;charset=utf8,<title>bug871156</title>\n" +
                   "<p>hello world";
  let firstTab = gBrowser.selectedTab;

  Services.prefs.setBoolPref("browser.tabs.animate", false);
  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("browser.tabs.animate");
  });

  yield loadTab(TEST_URI);

  let hud = yield openConsole();
  ok(hud, "Web Console opened");

  let tabClosed = promise.defer();
  let toolboxDestroyed = promise.defer();
  let tabSelected = promise.defer();

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let toolbox = gDevTools.getToolbox(target);

  gBrowser.tabContainer.addEventListener("TabClose", function onTabClose() {
    gBrowser.tabContainer.removeEventListener("TabClose", onTabClose);
    info("tab closed");
    tabClosed.resolve(null);
  });

  gBrowser.tabContainer.addEventListener("TabSelect", function onTabSelect() {
    gBrowser.tabContainer.removeEventListener("TabSelect", onTabSelect);
    if (gBrowser.selectedTab == firstTab) {
      info("tab selected");
      tabSelected.resolve(null);
    }
  });

  toolbox.once("destroyed", () => {
    info("toolbox destroyed");
    toolboxDestroyed.resolve(null);
  });

  
  executeSoon(() => {
    EventUtils.synthesizeKey("w", { accelKey: true });
  });

  yield promise.all([tabClosed.promise, toolboxDestroyed.promise,
                     tabSelected.promise]);
  info("promise.all resolved. start testing the Browser Console");

  hud = yield HUDService.toggleBrowserConsole();
  ok(hud, "Browser Console opened");

  let deferred = promise.defer();

  Services.obs.addObserver(function onDestroy() {
    Services.obs.removeObserver(onDestroy, "web-console-destroyed");
    ok(true, "the Browser Console closed");

    deferred.resolve(null);
  }, "web-console-destroyed", false);

  waitForFocus(() => {
    EventUtils.synthesizeKey("w", { accelKey: true }, hud.iframeWindow);
  }, hud.iframeWindow);

  yield deferred.promise;
});
