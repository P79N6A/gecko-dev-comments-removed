let testURL_click = "chrome://mochikit/content/browser/mobile/chrome/browser_click_content.html";

let newTab;
let element;
let isClickFired = false;
let clickPosition = { x: null, y: null};



function test() {
  
  waitForExplicitFinish();

  
  newTab = Browser.addTab(testURL_click, true);
  ok(newTab, "Tab Opened");

  
  waitFor(testClickAndPosition, function() { return newTab.isLoading() == false; });
}

function clickFired(aEvent) {
  isClickFired = true;
  let [x, y] = Browser.browserViewToClient(aEvent.clientX, aEvent.clientY);
  clickPosition.x = x;
  clickPosition.y = y;
}

function testClickAndPosition() {
  
  let uri = newTab.browser.currentURI.spec;
  is(uri, testURL_click, "URL Matches newly created Tab");

  
  element = newTab.browser.contentDocument.querySelector("iframe");
  element.addEventListener("click", clickFired, true);

  EventUtils.synthesizeMouseForContent(element, 1, 1, {}, window);
  waitFor(checkClick, function() { return isClickFired });
}

function checkClick() {
  ok(isClickFired, "Click handler fired");
  element.removeEventListener("click", clickFired, true);

  
  isClickFired = false;
  element = newTab.browser.contentDocument.documentElement;
  element.addEventListener("click", clickFired, true);
  let rect = Browser.getBoundingContentRect(element);
  EventUtils.synthesizeMouse(element, 1, rect.height + 10, {}, window);
  waitFor(checkPosition, function() { return isClickFired });
}

function checkPosition() {
  element.removeEventListener("click", clickFired, true);

  let rect = Browser.getBoundingContentRect(element);
  is(clickPosition.x, 1, "X position is correct");
  is(clickPosition.y, rect.height + 10, "Y position is correct");

  close();
}

function close() {
  
  Browser.closeTab(newTab);

  
  finish();
}
