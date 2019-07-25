


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
  whenSearchEnabledAndDisabled(testTwo);
  
  EventUtils.synthesizeKey("f", {accelKey: true}, cw);
}

function testTwo() {
  whenSearchEnabledAndDisabled(testThree);
  
  EventUtils.synthesizeKey("VK_SLASH", {}, cw);
}

function testThree() {
  let onTabViewShown = function () {
    is(cw.UI.getActiveTab(), groupItem.getChild(0), 
       "The active tab is newly created tab item");

    let onSearchEnabled = function () {
      let doc = cw.document;
      let searchBox = cw.iQ("#searchbox");
      let hasFocus = doc.hasFocus() && doc.activeElement == searchBox[0];
      ok(hasFocus, "The search box has focus");

      let tab = win.gBrowser.tabs[1];
      searchBox.val(tab._tabViewTabItem.$tabTitle[0].innerHTML);

      cw.performSearch();

      whenTabViewIsHidden(function () {
        is(tab, win.gBrowser.selectedTab, "The search result tab is shown");
        waitForFocus(finish);
      }, win);

      
      win.document.getElementById("menu_tabview").doCommand();
    };

    whenSearchEnabled(onSearchEnabled);
    EventUtils.synthesizeKey("VK_SLASH", {}, cw);
  };

  whenTabViewIsHidden(function () {
    showTabView(onTabViewShown, win);
  }, win);

  
  let groupItem = createEmptyGroupItem(cw, 300, 300, 200);
  let newTabButton = groupItem.container.getElementsByClassName("newTabButton");
  ok(newTabButton[0], "New tab button exists");

  EventUtils.sendMouseEvent({type: "click"}, newTabButton[0], cw);
}

function whenSearchEnabledAndDisabled(callback) {
  whenSearchEnabled(function () {
    whenSearchDisabled(callback);
    cw.hideSearch();
  });
}

function whenSearchEnabled(callback) {
  cw.addEventListener("tabviewsearchenabled", function onSearchEnabled() {
    cw.removeEventListener("tabviewsearchenabled", onSearchEnabled, false);
    callback();
  }, false);
}

function whenSearchDisabled(callback) {
  cw.addEventListener("tabviewsearchdisabled", function onSearchDisabled() {
    cw.removeEventListener("tabviewsearchdisabled", onSearchDisabled, false);
    callback();
  }, false);
}
