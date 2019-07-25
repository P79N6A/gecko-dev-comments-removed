


function test() {
  let cw;
  let win;
  let groupItem;

  let next = function () {
    let test = tests.shift();

    if (test) {
      test();
      return;
    }

    win.close();
    finish();
  }

  let prepareTest = function (testName) {
    let originalBounds = groupItem.getChild(0).getBounds();

    let tabItem = groupItem.getChild(1);
    let bounds = tabItem.getBounds();
    tabItem.close();

    ok(originalBounds.equals(groupItem.getChild(0).getBounds()), testName + ': tabs did not change their size');
    ok(bounds.equals(groupItem.getChild(1).getBounds()), testName + ': third tab is now on second tab\'s previous position');
    
    return originalBounds;
  }

  let cleanUpTest = function (testName, originalBounds, callback) {
    
    
    win.setTimeout(function () {
      ok(!originalBounds.equals(groupItem.getChild(0).getBounds()), testName + ': tabs changed their size');

      
      cw.UI.setActive(groupItem);
      win.gBrowser.loadOneTab('about:blank', {inBackground: true});
      afterAllTabsLoaded(callback, win);
    }, 500);
  }

  let tests = [];

  
  let testFocusTitle = function () {
    let originalBounds = prepareTest('testFocusTitle');

    let target = groupItem.$titleShield[0];
    EventUtils.synthesizeMouseAtCenter(target, {}, cw);

    cleanUpTest('testFocusTitle', originalBounds, next);
  }

  
  let testHideTabView = function () {
    let originalBounds = prepareTest('testHideTabView');

    hideTabView(function () {
      cleanUpTest('testHideTabView', originalBounds, function () {
        showTabView(next, win);
      });
    }, win);
  }

  
  let testCloseGroupUndo = function () {
    let originalBounds = prepareTest('testCloseGroupUndo');

    hideGroupItem(groupItem, function () {
      unhideGroupItem(groupItem, function () {
        cleanUpTest('testCloseGroupUndo', originalBounds, next);
      });
    });
  }

  
  let testMouseOut = function () {
    let originalBounds = prepareTest('testMouseOut');
    let doc = cw.document.documentElement;
    let bounds = groupItem.getBounds();

    EventUtils.synthesizeMouse(doc, bounds.right - 5, bounds.bottom - 5, {type: 'mousemove'}, cw);
    ok(originalBounds.equals(groupItem.getChild(0).getBounds()), 'testMouseOut: tabs did not change their size');

    EventUtils.synthesizeMouse(doc, bounds.right + 1, bounds.bottom + 1, {type: 'mousemove'}, cw);
    cleanUpTest('testMouseOut', originalBounds, next);
  }

  
  let testSortInGroup = function () {
    let originalBounds = prepareTest('testSortInGroup');
    let target = groupItem.getChild(0).container;

    
    EventUtils.synthesizeMouse(target, 20, 20, {type: 'mousedown'}, cw);
    EventUtils.synthesizeMouse(target, 40, 20, {type: 'mousemove'}, cw);
    EventUtils.synthesizeMouse(target, 20, 20, {type: 'mouseup'}, cw);

    cleanUpTest('testSortInGroup', originalBounds, next);
  }

  
  let testResizeGroup = function () {
    let originalBounds = prepareTest('testResizeGroup');
    let oldBounds = groupItem.getBounds();
    let resizer = groupItem.$resizer[0];

    
    EventUtils.synthesizeMouse(resizer, 5, 5, {type: 'mousedown'}, cw);
    EventUtils.synthesizeMouse(resizer, 40, 20, {type: 'mousemove'}, cw);
    EventUtils.synthesizeMouse(resizer, 20, 20, {type: 'mouseup'}, cw);

    
    groupItem.setBounds(oldBounds);
    groupItem.setUserSize();

    cleanUpTest('testResizeGroup', originalBounds, next);
  }

  
  let testRemoveWhileStacked = function () {
    let oldBounds = groupItem.getBounds();
    groupItem.setSize(250, 250, true);
    groupItem.setUserSize();

    let originalBounds = groupItem.getChild(0).getBounds();
    ok(!groupItem.isStacked(), 'testRemoveWhileStacked: group is not stacked');

    
    while (!groupItem.isStacked())
      win.gBrowser.loadOneTab('about:blank', {inBackground: true});

    afterAllTabsLoaded(function () {
      groupItem.getChild(0).close();
      ok(!groupItem.isStacked(), 'testRemoveWhileStacked: group is not stacked');

      let bounds = groupItem.getChild(0).getBounds();
      ok(originalBounds.equals(bounds), 'testRemoveWhileStacked: tabs did not change their size');

      
      groupItem.setBounds(oldBounds);
      groupItem.setUserSize();

      next();
    }, win);
  }

  
  
  
  let testExpandedMode = function () {
    let oldBounds = groupItem.getBounds();
    groupItem.setSize(100, 100, true);
    groupItem.setUserSize();

    ok(groupItem.isStacked(), 'testExpandedMode: group is stacked');

    groupItem.addSubscriber(groupItem, 'expanded', function () {
      groupItem.removeSubscriber(groupItem, 'expanded');
      onExpanded();
    });

    groupItem.addSubscriber(groupItem, 'collapsed', function () {
      groupItem.removeSubscriber(groupItem, 'collapsed');
      onCollapsed();
    });

    let onExpanded = function () {
      let originalBounds = groupItem.getChild(0).getBounds();
      let tabItem = groupItem.getChild(1);
      let bounds = tabItem.getBounds();

      while (groupItem.getChildren().length > 2)
        groupItem.getChild(1).close();

      ok(originalBounds.equals(groupItem.getChild(0).getBounds()), 'testExpandedMode: tabs did not change their size');

      
      let trayBounds = groupItem.expanded.bounds;
      let target = groupItem.expanded.$tray[0];
      EventUtils.synthesizeMouse(target, trayBounds.right - 5, trayBounds.bottom -5, {type: 'mousemove'}, cw);

      ok(originalBounds.equals(groupItem.getChild(0).getBounds()), 'testExpandedMode: tabs did not change their size');
      groupItem.collapse();
    }

    let onCollapsed = function () {
      
      groupItem.setBounds(oldBounds);
      groupItem.setUserSize();

      next();
    }

    groupItem.expand();
  }

  tests.push(testFocusTitle);
  tests.push(testHideTabView);
  tests.push(testCloseGroupUndo);
  tests.push(testMouseOut);
  tests.push(testSortInGroup);
  tests.push(testResizeGroup);
  tests.push(testRemoveWhileStacked);
  tests.push(testExpandedMode);

  waitForExplicitFinish();

  newWindowWithTabView(function (tvwin) {
    win = tvwin;

    registerCleanupFunction(function () {
      if (!win.closed)
        win.close();
    });

    cw = win.TabView.getContentWindow();

    groupItem = cw.GroupItems.groupItems[0];
    groupItem.setSize(400, 200, true);
    groupItem.setUserSize();

    for (let i=0; i<3; i++)
      win.gBrowser.loadOneTab('about:blank', {inBackground: true});

    afterAllTabsLoaded(next, win);
  });
}
