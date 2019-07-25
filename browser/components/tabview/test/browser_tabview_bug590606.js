


let originalTab;
let newTabOne;
let groupItemTwoId;

function test() {
  waitForExplicitFinish();

  originalTab = gBrowser.visibleTabs[0];
  
  newTabOne = gBrowser.addTab();

  registerCleanupFunction(function() {
    while (gBrowser.tabs[1])
      gBrowser.removeTab(gBrowser.tabs[1]);
    hideTabView();
  });

  showTabView(function() {
    let contentWindow = TabView.getContentWindow();

    registerCleanupFunction(function() {
      let groupItem = contentWindow.GroupItems.groupItem(groupItemTwoId);
      if (groupItem)
        closeGroupItem(groupItem);
    });

    is(contentWindow.GroupItems.groupItems.length, 1, 
       "There is one group item on startup");
    let groupItemOne = contentWindow.GroupItems.groupItems[0];
    is(groupItemOne.getChildren().length, 2, 
       "There should be two tab items in that group.");
    is(gBrowser.selectedTab, groupItemOne.getChild(0).tab,
       "The currently selected tab should be the first tab in the groupItemOne");

    
    let groupItemTwo = createGroupItemWithBlankTabs(window, 300, 300, 200, 1);
    groupItemTwoId = groupItemTwoId;
    hideTabView(function() {
      
      testGroupSwitch(contentWindow, groupItemOne, groupItemTwo);
    });
  });
}

function testGroupSwitch(contentWindow, groupItemOne, groupItemTwo) {
  is(gBrowser.selectedTab, groupItemTwo.getChild(0).tab,
     "The currently selected tab should be the only tab in the groupItemTwo");

  
  let tabItem = contentWindow.GroupItems.getNextGroupItemTab(false);
  if (tabItem)
    gBrowser.selectedTab = tabItem.tab;
  is(gBrowser.selectedTab, groupItemOne.getChild(0).tab,
    "The currently selected tab should be the first tab in the groupItemOne");
  
  
  gBrowser.selectedTab = groupItemOne.getChild(1).tab;

  
  tabItem = contentWindow.GroupItems.getNextGroupItemTab(false);
  if (tabItem)
    gBrowser.selectedTab = tabItem.tab;
  is(gBrowser.selectedTab, groupItemTwo.getChild(0).tab,
    "The currently selected tab should be the only tab in the groupItemTwo");

  
  tabItem = contentWindow.GroupItems.getNextGroupItemTab(false);
  if (tabItem)
    gBrowser.selectedTab = tabItem.tab;
  is(gBrowser.selectedTab, groupItemOne.getChild(1).tab,
    "The currently selected tab should be the second tab in the groupItemOne");

  
  gBrowser.removeTab(groupItemTwo.getChild(0).tab);
  gBrowser.removeTab(newTabOne);

  finish();
}
