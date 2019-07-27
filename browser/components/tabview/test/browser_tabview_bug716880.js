


let contentWindow;
let pinnedTab;

function test() {
  waitForExplicitFinish();

  registerCleanupFunction(function() {
    while (gBrowser.tabs[1])
      gBrowser.removeTab(gBrowser.tabs[1]);
    hideTabView();
  });

  pinnedTab = gBrowser.addTab("about:blank");
  gBrowser.pinTab(pinnedTab);
  ok(pinnedTab.pinned, "Tab 1 is pinned");

  gBrowser.addTab("about:mozilla");
  showTabView(setup);
}

function setup() {
  let prefix = "setup: ";
  
  registerCleanupFunction(function() {
    let groupItem = contentWindow.GroupItems.groupItem(groupItemTwoId);
    if (groupItem)
      closeGroupItem(groupItem);
  });

  contentWindow = TabView.getContentWindow();
  let groupItemOne = contentWindow.GroupItems.groupItems[0];

  is(contentWindow.GroupItems.groupItems.length, 1,
    prefix + "There is only one group");

  is(groupItemOne.getChildren().length, 2,
    prefix + "The number of tabs in group one is 2");

  
  let groupItemTwo =
    createGroupItemWithTabs(window, 300, 300, 310, ["about:blank"]);
  let groupItemTwoId = groupItemTwo.id;

  
  
  groupItemTwo.newTab("about:blank");

  is(contentWindow.GroupItems.getActiveGroupItem(), groupItemTwo, 
     prefix + "The group two is the active group");
  
  is(contentWindow.UI.getActiveTab(), groupItemTwo.getChild(1), 
     prefix + "The second tab item in group two is active");

  hideTabView(function () { switchToURL(groupItemOne, groupItemTwo) } );
}


function switchToURL(groupItemOne, groupItemTwo) {
  let prefix = "after switching: ";

  




  
  gURLBar.value = "moz-action:switchtab," + JSON.stringify({url: "about:mozilla"});
  
  gURLBar.focus();
  
  EventUtils.synthesizeKey("VK_RETURN", {});

  
  EventUtils.synthesizeKey("1", { accelKey: true });

  
  
  is(contentWindow.GroupItems.getActiveGroupItem(), groupItemOne, 
    prefix + "The group one is the active group");

  is(groupItemOne.getChildren().length, 2,
    prefix + "The number of tabs in group one is 2");

  is(groupItemTwo.getChildren().length, 1,
    prefix + "The number of tabs in group two is 1");

  gBrowser.removeTab(pinnedTab);
  finish();
}