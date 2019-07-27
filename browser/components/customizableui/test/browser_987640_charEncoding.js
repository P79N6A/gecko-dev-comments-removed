



"use strict";

const TEST_PAGE = "http://mochi.test:8888/browser/browser/components/customizableui/test/support/test_967000_charEncoding_page.html";
let newTab = null;

add_task(function() {
  info("Check Character Encoding panel functionality");

  
  CustomizableUI.addWidgetToArea("characterencoding-button",
                                  CustomizableUI.AREA_PANEL);

  newTab = gBrowser.addTab(TEST_PAGE);
  yield promiseTabLoadEvent(gBrowser.selectedTab, TEST_PAGE);

  yield PanelUI.show();
  let charEncodingButton = document.getElementById("characterencoding-button");
  charEncodingButton.click();

  let characterEncodingView = document.getElementById("PanelUI-characterEncodingView");
  let checkedButtons = characterEncodingView.querySelectorAll("toolbarbutton[checked='true']");
  let initialEncoding = checkedButtons[0];
  is(initialEncoding.getAttribute("label"), "Unicode", "The unicode encoding is initially selected");

  
  let encodings = characterEncodingView.querySelectorAll("toolbarbutton");
  let newEncoding = encodings[0].hasAttribute("checked") ? encodings[1] : encodings[0];
  let tabLoadPromise = promiseTabLoadEvent(gBrowser.selectedTab, TEST_PAGE);
  newEncoding.click();
  yield tabLoadPromise;

  
  yield PanelUI.show();
  charEncodingButton.click();
  checkedButtons = characterEncodingView.querySelectorAll("toolbarbutton[checked='true']");
  let selectedEncodingName = checkedButtons[0].getAttribute("label");
  ok(selectedEncodingName != "Unicode", "The encoding was changed to " + selectedEncodingName);

  
  yield PanelUI.show();
  charEncodingButton.click();
  tabLoadPromise = promiseTabLoadEvent(gBrowser.selectedTab, TEST_PAGE);
  initialEncoding.click();
  yield tabLoadPromise;
  yield PanelUI.show();
  charEncodingButton.click();
  checkedButtons = characterEncodingView.querySelectorAll("toolbarbutton[checked='true']");
  is(checkedButtons[0].getAttribute("label"), "Unicode", "The encoding was reset to Unicode");
});

add_task(function asyncCleanup() {
  
  yield resetCustomization();
  ok(CustomizableUI.inDefaultState, "The UI is in default state again.");

  
  gBrowser.removeTab(newTab);
});
