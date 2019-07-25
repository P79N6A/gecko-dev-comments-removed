




































function test() {
  let cw;

  let createGroupItem = function () {
    let bounds = new cw.Rect(20, 20, 400, 200);
    let groupItem = new cw.GroupItem([], {bounds: bounds, immediately: true});
    cw.GroupItems.setActiveGroupItem(groupItem);

    for (let i=0; i<5; i++)
      gBrowser.loadOneTab('about:blank', {inBackground: true});

    return groupItem;
  }

  let assertCorrectItemOrder = function (items) {
    for (let i=1; i<items.length; i++) {
      if (items[i-1].tab._tPos > items[i].tab._tPos) {
        ok(false, 'tabs were correctly reordered');
        break;
      }
    }
  }

  let testVariousTabOrders = function () {
    let groupItem = createGroupItem();
    let [tab1, tab2, tab3, tab4, tab5] = groupItem.getChildren();

    
    let tests = [];
    tests.push([tab1, tab2, tab3, tab4, tab5]);
    tests.push([tab5, tab4, tab3, tab2, tab1]);
    tests.push([tab1, tab2, tab3, tab4]);
    tests.push([tab4, tab3, tab2, tab1]);
    tests.push([tab1, tab2, tab3]);
    tests.push([tab1, tab2, tab3]);
    tests.push([tab1, tab3, tab2]);
    tests.push([tab2, tab1, tab3]);
    tests.push([tab2, tab3, tab1]);
    tests.push([tab3, tab1, tab2]);
    tests.push([tab3, tab2, tab1]);
    tests.push([tab1, tab2]);
    tests.push([tab2, tab1]);
    tests.push([tab1]);

    
    
    tests.push([]);

    while (tests.length) {
      let test = tests.shift();

      
      let items = groupItem.getChildren();
      while (test.length < items.length)
        items[items.length-1].close();

      let orig = cw.Utils.copy(items);
      items.sort(function (a, b) test.indexOf(a) - test.indexOf(b));

      
      groupItem.reorderTabsBasedOnTabItemOrder();
      assertCorrectItemOrder(items);

      
      items.sort(function (a, b) orig.indexOf(a) - orig.indexOf(b));
      groupItem.reorderTabsBasedOnTabItemOrder();
    }

    testMoveBetweenGroups();
  }

  let testMoveBetweenGroups = function () {
    let groupItem = createGroupItem();
    let groupItem2 = createGroupItem();
    
    afterAllTabsLoaded(function () {
      
      let child = groupItem.getChild(2);
      groupItem.remove(child);

      groupItem2.add(child, {index: 3});
      groupItem2.reorderTabsBasedOnTabItemOrder();

      assertCorrectItemOrder(groupItem.getChildren());
      assertCorrectItemOrder(groupItem2.getChildren());

      
      child = groupItem2.getChild(1);
      groupItem2.remove(child);

      groupItem.add(child, {index: 1});
      groupItem.reorderTabsBasedOnTabItemOrder();

      assertCorrectItemOrder(groupItem.getChildren());
      assertCorrectItemOrder(groupItem2.getChildren());

      
      groupItem.addSubscriber(groupItem, 'groupHidden', function () {
        groupItem.removeSubscriber(groupItem, 'groupHidden');
        groupItem.closeHidden();
        groupItem2.closeAll();
      });

      groupItem2.addSubscriber(groupItem, 'groupHidden', function () {
        groupItem2.removeSubscriber(groupItem, 'groupHidden');
        groupItem2.closeHidden();
        hideTabView(finish);
      });

      groupItem.closeAll();
    });
  }

  waitForExplicitFinish();

  showTabView(function () {
    cw = TabView.getContentWindow();
    afterAllTabsLoaded(testVariousTabOrders);
  });
}
