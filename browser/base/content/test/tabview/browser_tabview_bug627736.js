




































function test() {
  waitForExplicitFinish();
  newWindowWithTabView(onTabViewWindowLoaded);
}

function onTabViewWindowLoaded(win) {
  ok(win.TabView.isVisible(), "Tab View is visible");

  let contentWindow = win.document.getElementById("tab-view").contentWindow;
  let [originalTab] = win.gBrowser.visibleTabs;
  let originalGroup = contentWindow.GroupItems.getActiveGroupItem();

  
  function openAndClose(groupOrTab, callback) {
    
    let group = new contentWindow.GroupItem([], {
      immediately: true,
      bounds: {left: 20, top: 20, width: 400, height: 400}
    });
    contentWindow.GroupItems.setActiveGroupItem(group);
    win.gBrowser.loadOneTab('about:blank', {inBackground: true});
  
    is(group.getChildren().length, 1, "The group has one child now.");
    let tab = group.getChild(0);
  
    function check() {
      if (groupOrTab == 'group') {
        group.removeSubscriber(group, "groupHidden", check);
        group.closeHidden();
      } else
        tab.removeSubscriber(tab, "tabRemoved", check);
  
      is(contentWindow.GroupItems.getActiveGroupItem(), originalGroup,
        "The original group is active.");
      is(contentWindow.UI.getActiveTab(), originalTab._tabViewTabItem,
        "The original tab is active");
  
      callback();
    }
  
    if (groupOrTab == 'group') {
      group.addSubscriber(group, "groupHidden", check);
      group.closeAll();
    } else {
      tab.addSubscriber(tab, "tabRemoved", check);
      tab.close();
    }
  }

  
  openAndClose("group", function postPhase1() {
    
    openAndClose("tab", function postPhase2() {
      win.close();
      finish();
    });
  });
}