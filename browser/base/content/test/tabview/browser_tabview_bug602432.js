


function test() {
  waitForExplicitFinish();
  newWindowWithTabView(onTabViewWindowLoaded);
}

let contentWindow = null;

function onTabViewWindowLoaded(win) {
  ok(win.TabView.isVisible(), "Tab View is visible");

  contentWindow = win.TabView.getContentWindow();

  
  
  let numTabs = 10;
  let groupItem = createGroupItemWithBlankTabs(win, 150, 150, 100,
    numTabs, false);

  
  ok(groupItem.isStacked(), "Group item is stacked");

  
  contentWindow.TabItems.pausePainting();
  let children = groupItem.getChildren();
  is(children.length, numTabs, "Correct number of tabitems created");
   
  let leftToUpdate = numTabs;
  let testFunc = function(tabItem) {
    tabItem.removeSubscriber(tabItem, "updated");
    if (--leftToUpdate>0)
      return;
    
    
    let earliest = children[0]._lastTabUpdateTime;
    for (let c=1; c<children.length; ++c)
      ok(earliest <= children[c]._lastTabUpdateTime,
        "Stacked group item update ("+children[c]._lastTabUpdateTime+") > first item ("+earliest+")");
    shutDown(win, groupItem);
  };

  for (let c=0; c<children.length; ++c) {
    let tabItem = children[c];
    tabItem.addSubscriber(tabItem, "updated", testFunc);
    contentWindow.TabItems.update(tabItem.tab);
  }

  
  contentWindow.TabItems.resumePainting();
}

function shutDown(win, groupItem) {
  
  let groupItemCount = contentWindow.GroupItems.groupItems.length;
  closeGroupItem(groupItem, function() {
    
    is(contentWindow.GroupItems.groupItems.length, --groupItemCount,
       "The number of groups is decreased by 1");
    let onTabViewHidden = function() {
      win.removeEventListener("tabviewhidden", onTabViewHidden, false);
      
      ok(!TabView.isVisible(), "Tab View is hidden");
      win.close();
      ok(win.closed, "new window is closed");
      finish();
    };
    win.addEventListener("tabviewhidden", onTabViewHidden, false);
    win.TabView.toggle();
  });
}
