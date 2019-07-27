


function startSimpleClickTestCase(aTestNumber) {
  info("browser_referrer_simple_click: " +
       getReferrerTestDescription(aTestNumber));
  BrowserTestUtils.browserLoaded(gTestWindow.gBrowser.selectedBrowser).then(function() {
    checkReferrerAndStartNextTest(aTestNumber, null, null,
                                  startSimpleClickTestCase);
  });

  clickTheLink(gTestWindow, "testlink", {});
};

function test() {
  requestLongerTimeout(10);  
  startReferrerTest(startSimpleClickTestCase);
}
