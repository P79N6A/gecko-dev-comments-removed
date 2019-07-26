

let contentWindow;
let contentElement;
let groupItem;

function test() {
  waitForExplicitFinish();

  registerCleanupFunction(function () {
    hideTabView();
  });

  showTabView(function() {
    contentWindow = TabView.getContentWindow();
    contentElement = contentWindow.document.getElementById("content");
    test1();
  });
}

function test1() {
  is(gBrowser.tabs.length, 1, 
     "Total number of tabs is 1 before right button double click");
  
  mouseClick(contentElement, 2);
  
  mouseClick(contentElement, 2);

  is(gBrowser.tabs.length, 1, 
     "Total number of tabs is 1 after right button double click");
  test2();
}


function test2() {
  is(gBrowser.tabs.length, 1, 
     "Total number of tabs is 1 before left, right and left mouse clicks");

  
  mouseClick(contentElement, 0);
  
  mouseClick(contentElement, 2);
  
  mouseClick(contentElement, 0);

  is(gBrowser.tabs.length, 1, 
     "Total number of tabs is 1 before left, right and left mouse clicks");
  test3();
}

function test3() {
  ok(contentWindow.GroupItems.groupItems.length, 1, "Only one group item exists");
  groupItem = contentWindow.GroupItems.groupItems[0];

  is(groupItem.getChildren().length, 1, 
     "The number of tab items in the group is 1 before right button double click");

  
  mouseClick(groupItem.container, 2);
  
  mouseClick(groupItem.container, 2);

  is(groupItem.getChildren().length, 1, 
     "The number of tab items in the group is 1 after right button double click");
  test4();
}

function test4() {
  is(groupItem.getChildren().length, 1, 
     "The number of tab items in the group is 1 before left, right, left mouse clicks");

  
  mouseClick(groupItem.container, 0);
  
  mouseClick(groupItem.container, 2);
  
  mouseClick(groupItem.container, 0);

  is(groupItem.getChildren().length, 1, 
     "The number of tab items in the group is 1 before left, right, left mouse clicks");

  hideTabView(function() {
    is(gBrowser.tabs.length, 1, "Total number of tabs is 1 after all tests");

    contentWindow = null;
    contentElement = null;
    groupItem = null;

    finish();
  });
}

function mouseClick(targetElement, buttonCode) {
  EventUtils.sendMouseEvent(
    { type: "mousedown", button: buttonCode }, targetElement, contentWindow);
  EventUtils.sendMouseEvent(
    { type: "mouseup", button: buttonCode }, targetElement, contentWindow);
}

