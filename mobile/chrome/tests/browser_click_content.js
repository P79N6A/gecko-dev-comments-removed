let testURL_click = "chrome://mochikit/content/browser/mobile/chrome/browser_click_content.html";

let newTab;
let element;
let isClickFired = false;



function test() {
  
  waitForExplicitFinish();

  
  newTab = Browser.addTab(testURL_click, true);
  ok(newTab, "Tab Opened");

  
  waitFor(testClick, function() { return newTab.isLoading() == false; });
}

function clickFired() {
  isClickFired = true;
}

function testClick() {
  
  let uri = newTab.browser.currentURI.spec;
  is(uri, testURL_click, "URL Matches newly created Tab");

  
  element = newTab.browser.contentDocument.querySelector("iframe");
  element.addEventListener("click", clickFired, true);
  EventUtils.synthesizeMouseForContent(element, 1, 1, {}, window);
  waitFor(checkClick, function() { return isClickFired });
}

function checkClick() {
  ok(isClickFired, "Click handler fired");
  close();
}

function close() {
  
  Browser.closeTab(newTab);

  
  element.removeEventListener("click", clickFired, true);

  
  finish();
}
