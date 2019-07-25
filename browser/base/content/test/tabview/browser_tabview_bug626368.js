


function test() {
  let cw;

  let createGroupItem = function () {
    let bounds = new cw.Rect(20, 20, 150, 150);
    let groupItem = new cw.GroupItem([], {bounds: bounds, immediately: true});

    cw.UI.setActive(groupItem);
    gBrowser.loadOneTab('about:blank', {inBackground: true});

    return groupItem;
  }

  let synthesizeMiddleMouseDrag = function (tabContainer, width) {
    EventUtils.synthesizeMouseAtCenter(tabContainer,
        {type: 'mousedown', button: 1}, cw);
    let rect = tabContainer.getBoundingClientRect();
    EventUtils.synthesizeMouse(tabContainer, rect.width / 2 + width,
        rect.height / 2, {type: 'mousemove', button: 1}, cw);
    EventUtils.synthesizeMouse(tabContainer, rect.width / 2 + width,
        rect.height / 2, {type: 'mouseup', button: 1}, cw);
  }

  let testDragAndDropWithMiddleMouseButton = function () {
    let groupItem = createGroupItem();
    let tabItem = groupItem.getChild(0);
    let tabContainer = tabItem.container;
    let bounds = tabItem.getBounds();

    
    synthesizeMiddleMouseDrag(tabContainer, 200);
    is(groupItem.getChild(0), tabItem, 'tabItem was not closed');
    ok(bounds.equals(tabItem.getBounds()), 'bounds did not change');

    
    synthesizeMiddleMouseDrag(tabContainer, 10);
    ok(!groupItem.getChild(0), 'tabItem was closed');

    closeGroupItem(groupItem, function () {
      hideTabView(finish);
    });
  }

  waitForExplicitFinish();

  showTabView(function () {
    cw = TabView.getContentWindow();
    afterAllTabsLoaded(testDragAndDropWithMiddleMouseButton);
  });
}
