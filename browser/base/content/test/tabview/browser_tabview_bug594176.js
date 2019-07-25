




































function test() {
  let [origTab] = gBrowser.visibleTabs;
  ok(!origTab.pinned, "The original tab is not pinned");
 
  let pinnedTab = gBrowser.addTab();
  gBrowser.pinTab(pinnedTab);
  ok(pinnedTab.pinned, "The new tab is pinned");

  popup(origTab);
  ok(!document.getElementById("context_tabViewMenu").disabled, 
     "The tab view menu is enabled for normal tab");

  popup(pinnedTab);
  ok(document.getElementById("context_tabViewMenu").disabled, 
     "The tab view menu is disabled for pinned tab");

  gBrowser.unpinTab(pinnedTab);
  popup(pinnedTab);
  ok(!document.getElementById("context_tabViewMenu").disabled, 
     "The tab view menu is enabled for unpinned tab");

  gBrowser.removeTab(pinnedTab);
}

function popup(tab) {
  document.popupNode = tab;
  TabContextMenu.updateContextMenu(document.getElementById("tabContextMenu"));
}
