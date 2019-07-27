


function startNewTabTestCase(aTestNumber) {
  info("browser_referrer_open_link_in_tab: " +
       getReferrerTestDescription(aTestNumber));
  contextMenuOpened(gTestWindow, "testlink").then(function(aContextMenu) {
    someTabLoaded(gTestWindow).then(function(aNewTab) {
      gTestWindow.gBrowser.selectedTab = aNewTab;
      checkReferrerAndStartNextTest(aTestNumber, null, aNewTab,
                                    startNewTabTestCase);
    });

    doContextMenuCommand(gTestWindow, aContextMenu, "context-openlinkintab");
  });
}

function test() {
  requestLongerTimeout(10);  
  startReferrerTest(startNewTabTestCase);
}
