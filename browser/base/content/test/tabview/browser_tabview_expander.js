


function test() {
  waitForExplicitFinish();
  requestLongerTimeout(2);
  newWindowWithTabView(onTabViewWindowLoaded);
}

function onTabViewWindowLoaded(win) {
  ok(win.TabView.isVisible(), "Tab View is visible");

  let contentWindow = win.document.getElementById("tab-view").contentWindow;
  let [originalTab] = win.gBrowser.visibleTabs;
  let currentGroup = contentWindow.GroupItems.getActiveGroupItem();
  
  
  let group = new contentWindow.GroupItem([], {
    immediately: true,
    bounds: {left: 20, top: 300, width: 400, height: 400}
  });

  let expander = contentWindow.iQ(group.container).find(".stackExpander");
  ok("length" in expander && expander.length == 1, "The group has an expander.");

  
  contentWindow.GroupItems.setActiveGroupItem(group);
  for (var i=0; i<7; i++) {
    win.gBrowser.loadOneTab('about:blank#' + i, {inBackground: true});
  }
  let children = group.getChildren();
  
  
  
  afterAllTabItemsUpdated(function() {
    
    ok(!group.shouldStack(group._children.length), "The group should not stack.");
    is(expander[0].style.display, "none", "The expander is hidden.");
    
    
    group.setSize(100, 100, true);
  
    ok(group.shouldStack(group._children.length), "The group should stack.");
    isnot(expander[0].style.display, "none", "The expander is now visible!");
    let expanderBounds = expander.bounds();
    ok(group.getBounds().contains(expanderBounds), "The expander lies in the group.");
    let stackCenter = children[0].getBounds().center();
    ok(stackCenter.y < expanderBounds.center().y, "The expander is below the stack.");
  
    
    
    
    let stage1expanded = function() {
      group.removeSubscriber("test stage 1", "expanded", stage1expanded);
    
      ok(group.expanded, "The group is now expanded.");
      is(expander[0].style.display, "none", "The expander is hidden!");
      
      let overlay = contentWindow.document.getElementById("expandedTray");    
      ok(overlay, "The expanded tray exists.");
      let $overlay = contentWindow.iQ(overlay);
      
      group.addSubscriber("test stage 1", "collapsed", stage1collapsed);
      
      EventUtils.synthesizeMouse(contentWindow.document.body, 10, $overlay.bounds().bottom + 5,
                                 {type: null}, contentWindow);
    };
    
    let stage1collapsed = function() {
      group.removeSubscriber("test stage 1", "collapsed", stage1collapsed);
      ok(!group.expanded, "The group is no longer expanded.");
      isnot(expander[0].style.display, "none", "The expander is visible!");
      let expanderBounds = expander.bounds();
      ok(group.getBounds().contains(expanderBounds), "The expander still lies in the group.");
      let stackCenter = children[0].getBounds().center();
      ok(stackCenter.y < expanderBounds.center().y, "The expander is below the stack.");
  
      
      group.addSubscriber("test stage 2", "expanded", stage2expanded);
      EventUtils.sendMouseEvent({ type: "click" }, expander[0], contentWindow);
    };
  
    
    
    
    let stage2expanded = function() {
      group.removeSubscriber("test stage 2", "expanded", stage2expanded);
    
      ok(group.expanded, "The group is now expanded.");
      is(expander[0].style.display, "none", "The expander is hidden!");
      
      let overlay = contentWindow.document.getElementById("expandedTray");    
      ok(overlay, "The expanded tray exists.");
      let $overlay = contentWindow.iQ(overlay);
      let overlayBounds = $overlay.bounds();
  
      children.forEach(function(child, i) {
        ok(overlayBounds.contains(child.getBounds()), "Child " + i + " is in the overlay");
      });
      
      win.addEventListener("tabviewhidden", stage2hidden, false);
      
      EventUtils.synthesizeMouse(children[1].container, 2, 2, {type: null}, contentWindow);
    };
  
    let stage2hidden = function() {
      win.removeEventListener("tabviewhidden", stage2hidden, false);
      
      is(win.gBrowser.selectedTab, children[1].tab, "We clicked on the second child.");
      
      win.addEventListener("tabviewshown", stage2shown, false);
      win.TabView.toggle();
    };
    
    let stage2shown = function() {
      win.removeEventListener("tabviewshown", stage2shown, false);
      ok(!group.expanded, "The group is not expanded.");
      isnot(expander[0].style.display, "none", "The expander is visible!");
      let expanderBounds = expander.bounds();
      ok(group.getBounds().contains(expanderBounds), "The expander still lies in the group.");
      let stackCenter = children[0].getBounds().center();
      ok(stackCenter.y < expanderBounds.center().y, "The expander is below the stack.");

      is(group.topChild, children[1], "The top child in the stack is the second tab item");
      let topChildzIndex = children[1].zIndex;
      
      
      for (let i = 0; i < 6; i++) {
        if (i != 1)
          ok(children[i].zIndex < topChildzIndex,
            "The child[" + i + "] has smaller zIndex than second dhild");
      }

      
      group.addSubscriber("test stage 3", "expanded", stage3expanded);
      EventUtils.sendMouseEvent({ type: "click" }, expander[0], contentWindow);
    }

    
    
    let stage3expanded = function() {
      group.removeSubscriber("test stage 3", "expanded", stage3expanded);

      ok(group.expanded, "The group is now expanded.");
      let overlay = contentWindow.document.getElementById("expandedTray");    
      let $overlay = contentWindow.iQ(overlay);

      group.addSubscriber("test stage 3", "collapsed", stage3collapsed);
      
      EventUtils.synthesizeMouse(contentWindow.document.body, 10, $overlay.bounds().bottom + 5,
                                 {type: null}, contentWindow);
    };

    let stage3collapsed = function() {
      group.removeSubscriber("test stage 3", "collapsed", stage3collapsed);

      ok(!group.expanded, "The group is no longer expanded.");
      isnot(expander[0].style.display, "none", "The expander is visible!");

      let stackCenter = children[0].getBounds().center();
      ok(stackCenter.y < expanderBounds.center().y, "The expander is below the stack.");

      is(group.topChild, children[1], 
         "The top child in the stack is still the second tab item");
      let topChildzIndex = children[1].zIndex;
      
      
      for (let i = 0; i < 6; i++) {
        if (i != 1)
          ok(children[i].zIndex < topChildzIndex,
            "The child[" + i + "] has smaller zIndex than second dhild after a collapse.");
      }

      
      let originalTabItem = originalTab._tabViewTabItem;
      contentWindow.UI.setActiveTab(originalTabItem);

      
      group.addSubscriber("test stage 4", "expanded", stage4expanded);
      EventUtils.sendMouseEvent({ type: "click" }, expander[0], contentWindow);
    };

    
    
    
    
    let stage4expanded = function() {
      group.removeSubscriber("test stage 4", "expanded", stage4expanded);
    
      ok(group.expanded, "The group is now expanded.");
      is(expander[0].style.display, "none", "The expander is hidden!");
      let overlay = contentWindow.document.getElementById("expandedTray");
      ok(overlay, "The expanded tray exists.");
      
      let activeTab = contentWindow.UI.getActiveTab();
      ok(activeTab, "There is an active tab.");
      let originalTabItem = originalTab._tabViewTabItem;

      isnot(activeTab, originalTabItem, "But it's not what it was a moment ago.");
      let someChildIsActive = group.getChildren().some(function(child)
                              child == activeTab);
      ok(someChildIsActive, "Now one of the children in the group is active.");
            
      
      win.addEventListener("tabviewhidden", stage4hidden, false);
      win.TabView.toggle();
    };
  
    let stage4hidden = function() {
      win.removeEventListener("tabviewhidden", stage4hidden, false);
      
      isnot(win.gBrowser.selectedTab, originalTab, "We did not enter the original tab.");

      let someChildIsSelected = group.getChildren().some(function(child)
                                  child.tab == win.gBrowser.selectedTab);
      ok(someChildIsSelected, "Instead we're in one of the stack's children.");
      
      win.addEventListener("tabviewshown", stage4shown, false);
      win.TabView.toggle();
    };
    
    let stage4shown = function() {
      win.removeEventListener("tabviewshown", stage4shown, false);
  
      let overlay = contentWindow.document.getElementById("expandedTray");
      ok(!group.expanded, "The group is no longer expanded.");
      isnot(expander[0].style.display, "none", "The expander is visible!");

      win.close();
      finish();
    }
  
    
    group.addSubscriber("test stage 1", "expanded", stage1expanded);
    EventUtils.sendMouseEvent({ type: "click" }, expander[0], contentWindow);
  }, win);
}
