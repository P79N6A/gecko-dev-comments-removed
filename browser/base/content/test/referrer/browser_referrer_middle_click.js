


function startMiddleClickTestCase(aTestNumber) {
  info("browser_referrer_middle_click: " +
       getReferrerTestDescription(aTestNumber));
  someTabLoaded(gTestWindow).then(function(aNewTab) {
    gTestWindow.gBrowser.selectedTab = aNewTab;
    checkReferrerAndStartNextTest(aTestNumber, null, aNewTab,
                                  startMiddleClickTestCase);
  });

  clickTheLink(gTestWindow, "testlink", {button: 1});
}

function test() {
  requestLongerTimeout(10);  
  startReferrerTest(startMiddleClickTestCase);
}
