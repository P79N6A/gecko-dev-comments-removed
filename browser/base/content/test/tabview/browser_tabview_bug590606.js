


let originalTab;
let newTabOne;

function test() {
  waitForExplicitFinish();

  originalTab = gBrowser.visibleTabs[0];
  
  newTabOne = gBrowser.addTab();

  let onTabviewShown = function() {
    window.removeEventListener("tabviewshown", onTabviewShown, false);

    let contentWindow = document.getElementById("tab-view").contentWindow;

    is(contentWindow.GroupItems.groupItems.length, 1, 
       "There is one group item on startup");
    let groupItemOne = contentWindow.GroupItems.groupItems[0];
    is(groupItemOne.getChildren().length, 2, 
       "There should be two tab items in that group.");
    is(gBrowser.selectedTab, groupItemOne.getChild(0).tab,
       "The currently selected tab should be the first tab in the groupItemOne");

    
    let groupItemTwo = createEmptyGroupItem(contentWindow, 300, 300, 200);

    let onTabViewHidden = function() {
      window.removeEventListener("tabviewhidden", onTabViewHidden, false);
      
      testGroupSwitch(contentWindow, groupItemOne, groupItemTwo);
    };
    window.addEventListener("tabviewhidden", onTabViewHidden, false);

    
    let newTabButton = groupItemTwo.container.getElementsByClassName("newTabButton");
    ok(newTabButton[0], "New tab button exists");
    EventUtils.sendMouseEvent({ type: "click" }, newTabButton[0], contentWindow);
  };
  window.addEventListener("tabviewshown", onTabviewShown, false);
  TabView.toggle();
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

  
  gBrowser.removeTab(newTabOne);
  closeGroupItem(groupItemTwo, finish);
}
