


let win;
let cw;

function test() {
  waitForExplicitFinish();

  let onLoad = function (tvwin) {
    win = tvwin;
    registerCleanupFunction(function () win.close());
    win.gBrowser.loadOneTab("http://mochi.test:8888/", {inBackground: true});
  };

  let onShow = function () {
    cw = win.TabView.getContentWindow();
    ok(win.TabView.isVisible(), "Tab View is visible");
    afterAllTabItemsUpdated(testOne, win);
  };

  newWindowWithTabView(onShow, onLoad);
}

function testOne() {
  hideSearchWhenSearchEnabled(testTwo);
  
  EventUtils.synthesizeKey("f", {accelKey: true}, cw);
}

function testTwo() {
  hideSearchWhenSearchEnabled(testThree);
  
  EventUtils.synthesizeKey("VK_SLASH", {}, cw);
}

function testThree() {
  ok(win.TabView.isVisible(), "Tab View is visible");
  
  let groupItem = createGroupItemWithBlankTabs(win, 300, 300, 200, 1);
  is(cw.UI.getActiveTab(), groupItem.getChild(0), 
     "The active tab is newly created tab item");

  whenSearchIsEnabled(function () {
    let doc = cw.document;
    let searchBox = cw.iQ("#searchbox");
    let hasFocus = doc.hasFocus() && doc.activeElement == searchBox[0];
    ok(hasFocus, "The search box has focus");

    let tab = win.gBrowser.tabs[1];
    searchBox.val(tab._tabViewTabItem.$tabTitle[0].innerHTML);

    cw.Search.perform();

    whenTabViewIsHidden(function () {
      is(tab, win.gBrowser.selectedTab, "The search result tab is shown");
      finish()
    }, win);

    
    win.document.getElementById("menu_tabview").doCommand();
  }, win);
  EventUtils.synthesizeKey("VK_SLASH", {}, cw);
}

function hideSearchWhenSearchEnabled(callback) {
  whenSearchIsEnabled(function() {
    hideSearch(callback, win);
  }, win);
}

