






































function createEmptyGroupItem(contentWindow, width, height, padding, noAnimation) {
  let pageBounds = contentWindow.Items.getPageBounds();
  pageBounds.inset(padding, padding);

  let box = new contentWindow.Rect(pageBounds);
  box.width = width;
  box.height = height;
  
  let immediately = noAnimation ? true: false;
  let emptyGroupItem = 
    new contentWindow.GroupItem([], { bounds: box, immediately: immediately });

  return emptyGroupItem;
}


function afterAllTabItemsUpdated(callback, win) {
  win = win || window;
  let tabItems = win.document.getElementById("tab-view").contentWindow.TabItems;

  for (let a = 0; a < win.gBrowser.tabs.length; a++) {
    let tabItem = win.gBrowser.tabs[a]._tabViewTabItem;
    if (tabItem)
      tabItems._update(win.gBrowser.tabs[a]);
  }
  callback();
}


function newWindowWithTabView(callback) {
  let win = window.openDialog(getBrowserURL(), "_blank", 
                              "chrome,all,dialog=no,height=800,width=800");
  let onLoad = function() {
    win.removeEventListener("load", onLoad, false);
    let onShown = function() {
      win.removeEventListener("tabviewshown", onShown, false);
      callback(win);
    };
    win.addEventListener("tabviewshown", onShown, false);
    win.TabView.toggle();
  }
  win.addEventListener("load", onLoad, false);
}


function afterAllTabsLoaded(callback, win) {
  win = win || window;

  let stillToLoad = 0;

  function onLoad() {
    this.removeEventListener("load", onLoad, true);
    stillToLoad--;
    if (!stillToLoad)
      callback();
  }

  for (let a = 0; a < win.gBrowser.tabs.length; a++) {
    let browser = win.gBrowser.tabs[a].linkedBrowser;
    if (browser.contentDocument.readyState != "complete") {
      stillToLoad++;
      browser.addEventListener("load", onLoad, true);
    }
  }

  if (!stillToLoad)
    callback();
}
