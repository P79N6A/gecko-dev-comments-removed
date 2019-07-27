const TEST_URL = "http://example.com/browser/browser/base/content/test/general/app_bug575561.html";

add_task(function*() {
  SimpleTest.requestCompleteLog();

  
  
  yield testLink(0, true, false);
  
  
  yield testLink(1, true, true);

  
  
  yield testLink(2, true, true);

  
  
  yield testLink(2, false, false);

  
  
  yield testLink(3, true, true);

  
  
  yield testLink(0, true, false, true);

  
  
  yield testLink(4, true, false);

  
  
  yield testLink(5, true, false);

  
  
  yield testLink(6, true, false);
});

let waitForPageLoad = Task.async(function*(browser, linkLocation) {
  yield waitForDocLoadComplete();

  is(browser.contentDocument.location.href, linkLocation, "Link should not open in a new tab");
});

let waitForTabOpen = Task.async(function*() {
  let event = yield promiseWaitForEvent(gBrowser.tabContainer, "TabOpen", true);
  ok(true, "Link should open a new tab");

  yield waitForDocLoadComplete(event.target.linkedBrowser);
  yield Promise.resolve();

  gBrowser.removeCurrentTab();
});

let testLink = Task.async(function*(aLinkIndex, pinTab, expectNewTab, testSubFrame) {
  let appTab = gBrowser.addTab(TEST_URL, {skipAnimation: true});
  if (pinTab)
    gBrowser.pinTab(appTab);
  gBrowser.selectedTab = appTab;

  yield waitForDocLoadComplete();

  let browser = appTab.linkedBrowser;
  if (testSubFrame)
    browser = browser.contentDocument.querySelector("iframe");

  let link = browser.contentDocument.querySelectorAll("a")[aLinkIndex];

  let promise;
  if (expectNewTab)
    promise = waitForTabOpen();
  else
    promise = waitForPageLoad(browser, link.href);

  info("Clicking " + link.textContent);
  link.click();

  yield promise;

  gBrowser.removeTab(appTab);
});
