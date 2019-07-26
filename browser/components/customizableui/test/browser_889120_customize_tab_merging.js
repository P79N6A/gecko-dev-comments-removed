



"use strict";

const kTestToolbarId = "test-empty-drag";



add_task(function CheckBasicCustomizeMode() {
  yield startCustomizing();
  ok(CustomizationHandler.isCustomizing(), "We should be in customize mode");
  yield endCustomizing();
  ok(!CustomizationHandler.isCustomizing(), "We should not be in customize mode");
});
add_task(function CheckQuickCustomizeModeSwitch() {
  let tab1 = gBrowser.addTab("about:newtab");
  gBrowser.selectedTab = tab1;
  let tab2 = gBrowser.addTab("about:customizing");
  let tab3 = gBrowser.addTab("about:newtab");
  gBrowser.selectedTab = tab2;
  try {
    yield waitForCondition(() => CustomizationHandler.isEnteringCustomizeMode);
  } catch (ex) {
    Cu.reportError(ex);
  }
  ok(CustomizationHandler.isEnteringCustomizeMode, "Should be entering customize mode");
  gBrowser.selectedTab = tab3;
  try {
    yield waitForCondition(() => !CustomizationHandler.isEnteringCustomizeMode && !CustomizationHandler.isCustomizing());
  } catch (ex) {
    Cu.reportError(ex);
  }
  ok(!CustomizationHandler.isCustomizing(), "Should not be entering customize mode");
  gBrowser.removeTab(tab1);
  gBrowser.removeTab(tab2);
  gBrowser.removeTab(tab3);
});

add_task(function asyncCleanup() {
  yield endCustomizing();
});

