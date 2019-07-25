









const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-bug-597756-reopen-closed-tab.html";

let newTabIsOpen = false;

function tabLoaded(aEvent) {
  gBrowser.selectedBrowser.removeEventListener(aEvent.type, tabLoaded, true);

  openConsole(gBrowser.selectedTab, function() {
    gBrowser.selectedBrowser.addEventListener("load", tabReloaded, true);
    expectUncaughtException();
    content.location.reload();
  });
}

function tabReloaded(aEvent) {
  gBrowser.selectedBrowser.removeEventListener(aEvent.type, tabReloaded, true);

  let hudId = HUDService.getHudIdByWindow(content);
  let HUD = HUDService.hudReferences[hudId];
  ok(HUD, "Web Console is open");

  waitForSuccess({
    name: "error message displayed",
    validatorFn: function() {
      return HUD.outputNode.textContent.indexOf("fooBug597756_error") > -1;
    },
    successFn: function() {
      if (newTabIsOpen) {
        finishTest();
        return;
      }
      closeConsole(gBrowser.selectedTab, function() {
        gBrowser.removeCurrentTab();

        let newTab = gBrowser.addTab();
        gBrowser.selectedTab = newTab;

        newTabIsOpen = true;
        gBrowser.selectedBrowser.addEventListener("load", tabLoaded, true);
        expectUncaughtException();
        content.location = TEST_URI;
      });
    },
    failureFn: finishTest,
  });
}

function test() {
  expectUncaughtException();
  addTab(TEST_URI);
  browser.addEventListener("load", tabLoaded, true);
}

