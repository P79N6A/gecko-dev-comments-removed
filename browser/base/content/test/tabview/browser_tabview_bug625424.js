




































function test() {
  let cw;

  let getGroupItem = function (index) {
    return cw.GroupItems.groupItems[index];
  }

  let createOrphan = function (callback) {
    let tab = gBrowser.loadOneTab('about:blank', {inBackground: true});
    afterAllTabsLoaded(function () {
      let tabItem = tab._tabViewTabItem;
      tabItem.parent.remove(tabItem);
      callback(tabItem);
    });
  }

  let hideGroupItem = function (groupItem, callback) {
    groupItem.addSubscriber(groupItem, 'groupHidden', function () {
      groupItem.removeSubscriber(groupItem, 'groupHidden');
      callback();
    });
    groupItem.closeAll();
  }

  let showGroupItem = function (groupItem, callback) {
    groupItem.addSubscriber(groupItem, 'groupShown', function () {
      groupItem.removeSubscriber(groupItem, 'groupShown');
      callback();
    });
    groupItem._unhide();
  }

  let assertNumberOfTabsInGroupItem = function (groupItem, numTabs) {
    is(groupItem.getChildren().length, numTabs,
        'there are ' + numTabs + ' tabs in this groupItem');
  }

  let testDragOnHiddenGroup = function () {
    createOrphan(function (orphan) {
      let groupItem = getGroupItem(0);
      hideGroupItem(groupItem, function () {
        let drag = orphan.container;
        let drop = groupItem.$undoContainer[0];

        assertNumberOfTabsInGroupItem(groupItem, 1);

        EventUtils.synthesizeMouseAtCenter(drag, {type: 'mousedown'});
        EventUtils.synthesizeMouseAtCenter(drop, {type: 'mousemove'});
        EventUtils.synthesizeMouseAtCenter(drop, {type: 'mouseup'});

        assertNumberOfTabsInGroupItem(groupItem, 1);

        showGroupItem(groupItem, function () {
          drop = groupItem.container;
          
          EventUtils.synthesizeMouseAtCenter(drag, {type: 'mousedown'});
          EventUtils.synthesizeMouseAtCenter(drop, {type: 'mousemove'});
          EventUtils.synthesizeMouseAtCenter(drop, {type: 'mouseup'});

          assertNumberOfTabsInGroupItem(groupItem, 2);
          
          orphan.close();
          hideTabView(finish);
        });
      });
    });
  }

  waitForExplicitFinish();

  showTabView(function () {
    cw = TabView.getContentWindow();
    testDragOnHiddenGroup();
  });
}
