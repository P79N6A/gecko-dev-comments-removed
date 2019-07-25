


function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);

  ok(TabView.isVisible(), "Tab View is visible");

  let [originalTab] = gBrowser.visibleTabs;
  let contentWindow = document.getElementById("tab-view").contentWindow;

  
  let box1 = new contentWindow.Rect(310, 10, 300, 300);
  let group1 = new contentWindow.GroupItem([], { bounds: box1 });
  ok(group1.isEmpty(), "This group is empty");
  contentWindow.GroupItems.setActiveGroupItem(group1);
  let tab1 = gBrowser.loadOneTab("about:blank#1", {inBackground: true});
  let tab1Item = tab1._tabViewTabItem;
  ok(group1.getChildren().some(function(child) child == tab1Item), "The tab was made in our new group");
  is(group1.getChildren().length, 1, "Only one tab in the first group");

  group1.addSubscriber(group1, "close", function() {
    group1.removeSubscriber(group1, "close");

    let onTabViewHidden = function() {
      window.removeEventListener("tabviewhidden", onTabViewHidden, false);
      
      ok(!TabView.isVisible(), "Tab View is hidden");
      finish();
    };
    window.addEventListener("tabviewhidden", onTabViewHidden, false);

    
    
    
    executeSoon(function() { 
      EventUtils.synthesizeKey("e", {accelKey : true, shiftKey: true}, contentWindow);
    });
  });

  group1.addSubscriber(group1, "groupHidden", function() {
    group1.removeSubscriber(group1, "groupHidden");

    
    let closeButton = group1.$undoContainer.find(".close");
    EventUtils.sendMouseEvent(
      { type: "click" }, closeButton[0], contentWindow);
  });

  
  group1.closeAll();
}

