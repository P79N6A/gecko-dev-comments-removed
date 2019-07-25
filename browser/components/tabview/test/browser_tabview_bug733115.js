


function test() {
  waitForExplicitFinish();

  showTabView(function() {
    registerCleanupFunction(function() {
      if (gBrowser.tabs[1])
        gBrowser.removeTab(gBrowser.tabs[1]);
      TabView.hide();
    });

    let contentWindow = TabView.getContentWindow();
    let groupItem = createEmptyGroupItem(contentWindow, 200, 200, 20);
    contentWindow.GroupItems.setActiveGroupItem(groupItem);

    
    is(groupItem.getChildren().length, 0, "This group doesn't have any tabitems");
    whenTabViewIsHidden(function() {
      is(groupItem.getChildren().length, 1, "This group has one tabitem");

      showTabView(function() {
        
        whenTabViewIsHidden(function() {
          let tabItems = groupItem.getChildren();
          is(tabItems.length, 1, "This group has one tabitem");
          is(tabItems[0].tab, gBrowser.selectedTab, "The same tab");

          tabItems[0].close();
          groupItem.close();

          finish();
        });
        EventUtils.synthesizeKey("VK_ENTER", {}, contentWindow);
      });
    });
    EventUtils.synthesizeKey("VK_ENTER", {}, contentWindow);
  });
}

