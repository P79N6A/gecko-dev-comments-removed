




































function test() {
  gPrefService.setBoolPref("browser.ctrlTab.previews", true);

  let [origTab] = gBrowser.visibleTabs;
  let tabOne = gBrowser.addTab();
  let tabTwo = gBrowser.addTab();

  
  pressCtrlTab();
  ok(ctrlTab.tabList.length, 3, "Show 3 tabs in tab preview");
  releaseCtrl();

  gBrowser.showOnlyTheseTabs([origTab]);
  pressCtrlTab();
  ok(ctrlTab.tabList.length, 1, "Show 1 tab in tab preview");
  ok(!ctrlTab.isOpen, "With 1 tab open, Ctrl+Tab doesn't open the preview panel");

  gBrowser.showOnlyTheseTabs([origTab, tabOne, tabTwo]);
  pressCtrlTab();
  ok(ctrlTab.isOpen, "With 3 tabs open, Ctrl+Tab does open the preview panel");
  releaseCtrl();

  
  gBrowser.removeTab(tabOne);
  gBrowser.removeTab(tabTwo);

  if (gPrefService.prefHasUserValue("browser.ctrlTab.previews"))
    gPrefService.clearUserPref("browser.ctrlTab.previews");
}

function pressCtrlTab(aShiftKey) {
  EventUtils.synthesizeKey("VK_TAB", { ctrlKey: true, shiftKey: !!aShiftKey });
}

function releaseCtrl() {
  EventUtils.synthesizeKey("VK_CONTROL", { type: "keyup" });
}
