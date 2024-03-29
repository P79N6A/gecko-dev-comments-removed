


function startNewPrivateWindowTestCase(aTestNumber) {
  info("browser_referrer_open_link_in_private: " +
       getReferrerTestDescription(aTestNumber));
  contextMenuOpened(gTestWindow, "testlink").then(function(aContextMenu) {
    newWindowOpened().then(function(aNewWindow) {
      someTabLoaded(aNewWindow).then(function() {
        checkReferrerAndStartNextTest(aTestNumber, aNewWindow, null,
                                      startNewPrivateWindowTestCase);
      });
    });

    doContextMenuCommand(gTestWindow, aContextMenu, "context-openlinkprivate");
  });
}

function test() {
  requestLongerTimeout(10);  
  startReferrerTest(startNewPrivateWindowTestCase);
}
