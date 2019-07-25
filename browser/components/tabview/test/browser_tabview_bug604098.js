


let contentWindow;
let contentElement;

function test() {
  waitForExplicitFinish();

  registerCleanupFunction(function() {
    if (gBrowser.tabs.length > 1)
      gBrowser.removeTab(gBrowser.tabs[1]);
    hideTabView();
  });

  showTabView(function() {
    contentWindow = TabView.getContentWindow();
    contentElement = contentWindow.document.getElementById("content");
    test1();
  });
}

function test1() {
  let groupItems = contentWindow.GroupItems.groupItems;
  is(groupItems.length, 1, "there is one groupItem");

  whenTabViewIsHidden(function() {
    gBrowser.selectedTab = gBrowser.tabs[0];
    is(groupItems.length, 2, "there are two groupItems");
    closeGroupItem(groupItems[1], finish);
  });

  
  doubleClick(contentElement, 0);
}

function doubleClick(targetElement, buttonCode) {
  EventUtils.sendMouseEvent(
    { type: "dblclick", button: buttonCode }, targetElement, contentWindow);
}

