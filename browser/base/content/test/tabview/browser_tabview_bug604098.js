


let contentWindow;
let contentElement;

function test() {
  waitForExplicitFinish();

  registerCleanupFunction(function() {
    if (gBrowser.tabs.length > 1)
      gBrowser.removeTab(gBrowser.tabs[1]);
    hideTabView(function() {});
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
    is(groupItems.length, 2, "there are two groupItems");
    closeGroupItem(groupItems[1], finish);
  });

  
  mouseClick(contentElement, 0);
  
  mouseClick(contentElement, 0);
}

function mouseClick(targetElement, buttonCode) {
  EventUtils.sendMouseEvent(
    { type: "mousedown", button: buttonCode }, targetElement, contentWindow);
  EventUtils.sendMouseEvent(
    { type: "mouseup", button: buttonCode }, targetElement, contentWindow);
}

