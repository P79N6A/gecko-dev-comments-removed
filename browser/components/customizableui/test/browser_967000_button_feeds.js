



"use strict";

const TEST_PAGE = "http://mochi.test:8888/browser/browser/components/customizableui/test/support/feeds_test_page.html";
const TEST_FEED = "http://mochi.test:8888/browser/browser/components/customizableui/test/support/test-feed.xml"

let newTab = null;
let initialLocation = gBrowser.currentURI.spec;

add_task(function() {
  info("Check Subscribe button functionality");

  
  CustomizableUI.addWidgetToArea("feed-button",
                                  CustomizableUI.AREA_PANEL);

  
  yield PanelUI.show();

  let feedButton = document.getElementById("feed-button");
  ok(feedButton, "The Subscribe button was added to the Panel Menu");
  is(feedButton.getAttribute("disabled"), "true", "The Subscribe button is initially disabled");

  let panelHidePromise = promisePanelHidden(window);
  PanelUI.hide();
  yield panelHidePromise;

  newTab = gBrowser.selectedTab;
  yield promiseTabLoadEvent(newTab, TEST_PAGE);

  yield PanelUI.show();

  yield waitForCondition(function() !feedButton.hasAttribute("disabled"));
  ok(!feedButton.hasAttribute("disabled"), "The Subscribe button gets enabled");

  feedButton.click();
  yield promiseTabLoadEvent(newTab, TEST_FEED);

  is(gBrowser.currentURI.spec, TEST_FEED, "Subscribe page opened");
  ok(!isPanelUIOpen(), "Panel is closed");

  if(isPanelUIOpen()) {
    panelHidePromise = promisePanelHidden(window);
    PanelUI.hide();
    yield panelHidePromise;
  }
});

add_task(function asyncCleanup() {
  
  yield resetCustomization();
  ok(CustomizableUI.inDefaultState, "The UI is in default state again.");

  
  gBrowser.addTab(initialLocation);
  gBrowser.removeTab(newTab);
});
