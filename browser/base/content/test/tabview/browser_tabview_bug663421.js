


function test() {
  let win, cw, groupItem;

  function checkNumberOfGroupItems(num) {
    is(cw.GroupItems.groupItems.length, num, "there are " + num + " groupItems");
  }

  function next() {
    if (tests.length)
      tests.shift()();
    else
      finish();
  }

  
  function test1() {
    hideTabView(function () {
      showTabView(function () {
        checkNumberOfGroupItems(2);
        next();
      }, win);
    }, win);
  }

  
  function test2() {
    whenTabViewIsHidden(function () {
      whenTabViewIsShown(function () {
        checkNumberOfGroupItems(2);
        next();
      }, win);

      win.gBrowser.removeTab(win.gBrowser.selectedTab);
    }, win);

    groupItem.newTab();
  }

  
  function test3() {
    whenTabViewIsHidden(function () {
      showTabView(function () {
        let tab = win.gBrowser.tabs[1];
        tab._tabViewTabItem.close();
        checkNumberOfGroupItems(1);
        next();
      }, win);
    }, win);

    win.gBrowser.addTab();
  }

  
  function test4() {
    groupItem = createGroupItemWithBlankTabs(win, 200, 200, 20, 1);
    checkNumberOfGroupItems(2);

    let tab = win.gBrowser.tabs[1];
    let target = tab._tabViewTabItem.container;

    waitForFocus(function () {
      EventUtils.synthesizeMouseAtCenter(target, {type: "mousedown"}, cw);
      EventUtils.synthesizeMouse(target, 600, 5, {type: "mousemove"}, cw);
      EventUtils.synthesizeMouse(target, 600, 5, {type: "mouseup"}, cw);

      checkNumberOfGroupItems(2);
      closeGroupItem(cw.GroupItems.groupItems[1], next);
    }, win);
  }

  let tests = [test1, test2, test3, test4];

  waitForExplicitFinish();

  newWindowWithTabView(function (aWin) {
    registerCleanupFunction(function () aWin.close());

    win = aWin;
    cw = win.TabView.getContentWindow();
    groupItem = createEmptyGroupItem(cw, 200, 200, 20);

    checkNumberOfGroupItems(2);
    next();
  });
}
