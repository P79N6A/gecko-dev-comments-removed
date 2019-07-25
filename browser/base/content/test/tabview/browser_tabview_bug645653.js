




















function loadedAboutMozilla(tab) {
  return tab.linkedBrowser.contentDocument.getElementById('moztext');
}

function test() {
  waitForExplicitFinish();
  showTabView(function() {
    ok(TabView.isVisible(), "Tab View is visible");

    let contentWindow = TabView.getContentWindow();
    is(contentWindow.GroupItems.groupItems.length, 1, "There is one group item on startup.");

    let originalGroupItem = contentWindow.GroupItems.groupItems[0];
    is(originalGroupItem.getChildren().length, 1, "There is one tab item in that group.");

    let originalTabItem = originalGroupItem.getChild(0);
    ok(originalTabItem, "The tabitem has been found.");

    
    originalGroupItem.close();
    contentWindow.UI.setActive(originalGroupItem);
    is(contentWindow.GroupItems.groupItems.length, 0, "There are not any groups now.");

    ok(TabView.isVisible(), "Tab View is still shown.");

    hideTabView(function() {
      ok(!TabView.isVisible(), "Tab View is not shown anymore.");

      
      loadURI('about:mozilla');

      afterAllTabsLoaded(function() {
        ok(loadedAboutMozilla(originalTabItem.tab), "The original tab loaded about:mozilla.");

        
        duplicateTabIn(originalTabItem.tab, "tabshift");

        afterAllTabsLoaded(function() {
          
          is(gBrowser.selectedTab, originalTabItem.tab, "The selected tab is the original one.");
          is(contentWindow.GroupItems.groupItems.length, 1, "There is one group item again.");
          let groupItem = contentWindow.GroupItems.groupItems[0];
          is(groupItem.getChildren().length, 2, "There are two tab items in that group.");
          is(originalTabItem, groupItem.getChild(0), "The first tab item in the group is the original one.");
          let otherTab = groupItem.getChild(1);
          ok(loadedAboutMozilla(otherTab.tab), "The other tab loaded about:mozilla.");

          
          gBrowser.removeTab(otherTab.tab);
          is(contentWindow.GroupItems.groupItems.length, 1, "There is one group item after closing the second tab.");
          is(groupItem.getChildren().length, 1, "There is only one tab item after closing the second tab.");
          is(originalTabItem, groupItem.getChild(0), "The first tab item in the group is still the original one.");
          loadURI("about:blank");
          afterAllTabsLoaded(function() {
            finish();
          });
        });
      });
    });
  });
}

