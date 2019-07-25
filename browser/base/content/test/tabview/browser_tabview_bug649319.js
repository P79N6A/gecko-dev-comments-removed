


function test() {
  waitForExplicitFinish();

  newWindowWithTabView(function (win) {
    registerCleanupFunction(function () win.close());
    waitForFocus(function () testScenarios(win));
  });
}

function testScenarios(win) {
  let simulateDragDrop = function (target) {
    EventUtils.synthesizeMouseAtCenter(target, {type: "mousedown"}, cw);
    EventUtils.synthesizeMouse(target, 40, 20, {type: "mousemove"}, cw);
    EventUtils.synthesizeMouse(target, 80, 20, {type: "mouseup"}, cw);
  }

  let dragOutOfGroup = function (target) {
    EventUtils.synthesizeMouseAtCenter(target, {type: "mousedown"}, cw);
    EventUtils.synthesizeMouse(target, 600, 5, {type: "mousemove"}, cw);
    EventUtils.synthesizeMouse(target, 600, 5, {type: "mouseup"}, cw);
  }

  let dragIntoGroup = function (target) {
    EventUtils.synthesizeMouseAtCenter(target, {type: "mousedown"}, cw);
    EventUtils.synthesizeMouse(target, -200, 5, {type: "mousemove"}, cw);
    EventUtils.synthesizeMouse(target, -200, 5, {type: "mouseup"}, cw);
  }

  let assertActiveOrphan = function (tabItem) {
    ok(!cw.GroupItems.getActiveGroupItem(), "no groupItem is active");
    is(cw.UI.getActiveTab(), tabItem, "orphan tab is active");
    is(cw.UI.getActiveOrphanTab(), tabItem, "orphan tab is active");
  }

  let cw = win.TabView.getContentWindow();
  let groupItem = cw.GroupItems.groupItems[0];
  let groupItem2 = createGroupItemWithBlankTabs(win, 400, 300, 20, 4);

  
  cw.UI.setActive(groupItem);
  simulateDragDrop(groupItem2.container);
  is(cw.GroupItems.getActiveGroupItem(), groupItem2, "second groupItem is active");
  is(cw.UI.getActiveTab(), groupItem2.getChild(0), "second groupItem's first tab is active");

  
  cw.UI.setActive(groupItem);
  let tabItem = groupItem2.getChild(2);
  groupItem2.setActiveTab(tabItem);
  simulateDragDrop(groupItem2.$resizer[0]);
  is(cw.GroupItems.getActiveGroupItem(), groupItem2, "second groupItem is active");
  is(cw.UI.getActiveTab(), tabItem, "second groupItem's third tab is active");

  
  tabItem = groupItem2.getChild(0);
  dragOutOfGroup(tabItem.container);

  
  cw.UI.setActive(groupItem2);
  simulateDragDrop(tabItem.container);
  assertActiveOrphan(tabItem);

  
  cw.UI.setActive(groupItem2);
  let $resizer = cw.iQ('.iq-resizable-handle', tabItem.container);
  simulateDragDrop($resizer[0]);
  assertActiveOrphan(tabItem);

  
  dragIntoGroup(tabItem.container);
  cw.UI.setActive(groupItem);
  cw.UI.setActive(groupItem2);
  is(cw.UI.getActiveTab(), tabItem, "the dropped tab is active");

  
  hideGroupItem(groupItem2, function () {
    is(cw.GroupItems.getActiveGroupItem(), groupItem, "first groupItem is active");

    unhideGroupItem(groupItem2, function () {
      is(cw.GroupItems.getActiveGroupItem(), groupItem2, "second groupItem is active");
      is(cw.UI.getActiveTab(), tabItem, "the dropped tab is active");

      finish();
    });
  });
}
