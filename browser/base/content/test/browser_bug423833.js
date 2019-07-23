


var invalidPage = 'http://127.0.0.1:55555/';
var validPage = 'http://example.com/';
var testPage = 'data:text/html,<frameset cols="400,400"><frame src="' + validPage + '"><frame src="' + invalidPage + '"></frameset>';


var test2tab;
var test3window;


var intervalID;

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", test1Setup, true);
  content.location = testPage;
}

function test1Setup() {
  if (content.frames.length < 2 ||
      content.frames[1].location != invalidPage)
    
    return;

  gBrowser.selectedBrowser.removeEventListener("load", test1Setup, true);

  var badFrame = content.frames[1];
  document.popupNode = badFrame.document.firstChild;

  var contentAreaContextMenu = document.getElementById("contentAreaContextMenu");
  var contextMenu = new nsContextMenu(contentAreaContextMenu, gBrowser);

  
  contextMenu.showOnlyThisFrame();
  intervalID = setInterval(testShowOnlyThisFrame, 3000);
}

function testShowOnlyThisFrame() {
  if (content.location.href == testPage)
    
    return;

  
  
  clearInterval(intervalID);

  is(content.location.href, invalidPage, "Should navigate to page url, not about:neterror");

  
  gBrowser.addEventListener("load", test2Setup, true);
  content.location = testPage;
}

function test2Setup() {
  if (content.frames.length < 2 ||
      content.frames[1].location != invalidPage)
    
    return;

  gBrowser.removeEventListener("load", test2Setup, true);

  
  var badFrame = content.frames[1];

  document.popupNode = badFrame.document.firstChild;

  var contentAreaContextMenu = document.getElementById("contentAreaContextMenu");
  var contextMenu = new nsContextMenu(contentAreaContextMenu, gBrowser);

  test2tab = contextMenu.openFrameInTab();
  ok(test2tab instanceof XULElement, "openFrameInTab() should return an element (non-null)");
  is(test2tab.tagName, "tab", "openFrameInTab() should return a *tab* element");

  gBrowser.selectedTab = test2tab;

  intervalID = setInterval(testOpenFrameInTab, 3000);
}

function testOpenFrameInTab() {
  if (gBrowser.contentDocument.location.href == "about:blank")
    
    return;

  clearInterval(intervalID);

  
  is(gBrowser.contentDocument.location.href, invalidPage, "New tab should have page url, not about:neterror");

  
  gBrowser.removeCurrentTab();

  test3Setup();
}

function test3Setup() {
  
  var badFrame = content.frames[1];
  document.popupNode = badFrame.document.firstChild;

  var contentAreaContextMenu = document.getElementById("contentAreaContextMenu");
  var contextMenu = new nsContextMenu(contentAreaContextMenu, gBrowser);

  test3window = contextMenu.openFrame();
  ok(test3window instanceof Window, "openFrame() should return a window (non-null) ");

  intervalID = setInterval(testOpenFrame, 3000);
}

function testOpenFrame() {
  if (test3window.content.location.href == "about:blank")
    
    return;

  clearInterval(intervalID);

  is(test3window.content.location.href, invalidPage, "New window should have page url, not about:neterror");

  test3window.close();
  cleanup();
}

function cleanup() {
  gBrowser.removeCurrentTab();
  finish();
}
