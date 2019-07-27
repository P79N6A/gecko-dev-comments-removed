


function startNewWindowTestCase(aTestNumber) {
  info("browser_referrer_open_link_in_window: " +
       getReferrerTestDescription(aTestNumber));
  contextMenuOpened(gTestWindow, "testlink").then(function(aContextMenu) {
    newWindowOpened().then(function(aNewWindow) {
      someTabLoaded(aNewWindow).then(function() {
        checkReferrerAndStartNextTest(aTestNumber, aNewWindow, null,
                                      startNewWindowTestCase);
      });
    });

    doContextMenuCommand(gTestWindow, aContextMenu, "context-openlink");
  });
}

function test() {
  requestLongerTimeout(5);  
  startReferrerTest(startNewWindowTestCase);
}
