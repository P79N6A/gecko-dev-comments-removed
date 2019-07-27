


"use strict";

const TEST_VALUE = "example.com/\xF7?\xF7";
const START_VALUE = "example.com/%C3%B7?%C3%B7";

add_task(function* () {
  info("Simple return keypress");
  let tab = gBrowser.selectedTab = gBrowser.addTab(START_VALUE);

  gURLBar.focus();
  EventUtils.synthesizeKey("VK_RETURN", {});
  yield BrowserTestUtils.browserLoaded(gBrowser.selectedBrowser);

  
  is(gURLBar.textValue, TEST_VALUE, "Urlbar should preserve the value on return keypress");
  is(gBrowser.selectedTab, tab, "New URL was loaded in the current tab");

  
  gBrowser.removeCurrentTab();
});

add_task(function* () {
  info("Alt+Return keypress");
  let tab = gBrowser.selectedTab = gBrowser.addTab(START_VALUE);

  gURLBar.focus();
  EventUtils.synthesizeKey("VK_RETURN", {altKey: true});
  yield BrowserTestUtils.browserLoaded(gBrowser.selectedBrowser);

  
  is(gURLBar.textValue, TEST_VALUE, "Urlbar should preserve the value on return keypress");
  isnot(gBrowser.selectedTab, tab, "New URL was loaded in a new tab");

  
  gBrowser.removeTab(tab);
  gBrowser.removeCurrentTab();
});
