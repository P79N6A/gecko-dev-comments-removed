


let newTabOne;
let newTabTwo;
let restoredNewTabOneLoaded = false;
let restoredNewTabTwoLoaded = false;
let frameInitialized = false;

function test() {
  waitForExplicitFinish();
  newWindowWithTabView(setupOne);
}

function setupOne(win) {
  win.TabView.firstUseExperienced = true;

  win.gBrowser.addTab("http://mochi.test:8888/browser/browser/base/content/test/tabview/search1.html");
  win.gBrowser.addTab("http://mochi.test:8888/browser/browser/base/content/test/tabview/dummy_page.html");

  afterAllTabsLoaded(function () setupTwo(win), win);
}

function setupTwo(win) {
  let contentWindow = win.TabView.getContentWindow();

  let tabItems = contentWindow.TabItems.getItems();
  is(tabItems.length, 3, "There should be 3 tab items before closing");

  let numTabsToSave = tabItems.length;

  
  tabItems.forEach(function(tabItem) {
    contentWindow.TabItems._update(tabItem.tab);
    tabItem.addSubscriber(tabItem, "savedCachedImageData", function(item) {
      item.removeSubscriber(item, "savedCachedImageData");
      --numTabsToSave;
    });
  });

  
  let xulWindowDestory = function() {
    Services.obs.removeObserver(
       xulWindowDestory, "xul-window-destroyed", false);

    
    
    executeSoon(function() {
      let restoredWin = undoCloseWindow();
      restoredWin.addEventListener("load", function onLoad(event) {
        restoredWin.removeEventListener("load", onLoad, false);

        
        is(numTabsToSave, 0, "All tabs were saved when window was closed.");

        
        let onTabViewFrameInitialized = function() {
          restoredWin.removeEventListener(
            "tabviewframeinitialized", onTabViewFrameInitialized, false);

          






          restoredWin.close();
          finish();
        }
        restoredWin.addEventListener(
          "tabviewframeinitialized", onTabViewFrameInitialized, false);

        is(restoredWin.gBrowser.tabs.length, 3, "The total number of tabs is 3");

        








      }, false);
    });
  };

  Services.obs.addObserver(
    xulWindowDestory, "xul-window-destroyed", false);

  win.close();
}


























































































