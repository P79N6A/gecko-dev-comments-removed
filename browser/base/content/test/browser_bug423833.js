


var invalidPage = 'http://127.0.0.1:55555/';
var validPage = 'http://example.com/';
var testPage = 'data:text/html,<frameset cols="400,400"><frame src="' + validPage + '"><frame src="' + invalidPage + '"></frameset>';


var newBrowser;


var test2tab;
var test3window;


var intervalID;

function test() {
  
  waitForExplicitFinish();
  
  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  newBrowser = gBrowser.getBrowserForTab(newTab);
  
  newBrowser.addEventListener("load", test1Setup, true);
  newBrowser.contentWindow.location = testPage;
}

var loadCount = 0;
function test1Setup() {
  if(!loadCount++)
    
    return;
  
  loadCount = 0;
  newBrowser.removeEventListener("load", test1Setup, true);

  var badFrame = newBrowser.contentWindow.frames[1];
  document.popupNode = badFrame.document.firstChild;
  
  var contentAreaContextMenu = document.getElementById("contentAreaContextMenu");
  var contextMenu = new nsContextMenu(contentAreaContextMenu, gBrowser);

  
  contextMenu.showOnlyThisFrame();
  intervalID = window.setInterval(testShowOnlyThisFrame, 3000);
}

function testShowOnlyThisFrame() {
  
  if(newBrowser.contentDocument.location.href == testPage)
    
    return;
  
  
  
  window.clearInterval(intervalID);
  
  is(newBrowser.contentDocument.location.href, invalidPage, "Should navigate to page url, not about:neterror");
  
  
  gBrowser.addEventListener("load", test2Setup, true);
  newBrowser.contentWindow.location = testPage;
}

function test2Setup() {
  if(!loadCount++)
    
    return;
  
  loadCount = 0;
  gBrowser.removeEventListener("load", test2Setup, true);
  
  
  newBrowser = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  var badFrame = newBrowser.contentWindow.frames[1];
  
  document.popupNode = badFrame.document.firstChild;
  
  var contentAreaContextMenu = document.getElementById("contentAreaContextMenu");
  var contextMenu = new nsContextMenu(contentAreaContextMenu, gBrowser);

  test2tab = contextMenu.openFrameInTab();
  ok(test2tab instanceof XULElement, "openFrameInTab() should return an element (non-null)");
  is(test2tab.tagName, "tab", "openFrameInTab() should return a *tab* element");
  
  gBrowser.selectedTab = test2tab;

  intervalID = window.setInterval(testOpenFrameInTab, 3000);
}

function testOpenFrameInTab() {
  
  if(gBrowser.contentDocument.location.href == "about:blank")
    
    return;
  window.clearInterval(intervalID);
  
  
  is(gBrowser.contentDocument.location.href, invalidPage, "New tab should have page url, not about:neterror");

  
  gBrowser.removeCurrentTab();

  test3Setup();
}

function test3Setup() {

  
  newBrowser = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  var badFrame = newBrowser.contentWindow.frames[1];
  document.popupNode = badFrame.document.firstChild;
  
  var contentAreaContextMenu = document.getElementById("contentAreaContextMenu");
  var contextMenu = new nsContextMenu(contentAreaContextMenu, gBrowser);

  test3window = contextMenu.openFrame();
  ok(test3window instanceof Window, "openFrame() should return a window (non-null) ");
  
  intervalID = window.setInterval(testOpenFrame, 3000);
}

function testOpenFrame() {
  
  if(test3window.content.document.location.href == "about:blank")
    
    return;
  window.clearInterval(intervalID);
     
  is(test3window.content.document.location.href, invalidPage, "New window should have page url, not about:neterror");
  
  test3window.close();
  cleanup();
}

function cleanup() {
  gBrowser.removeCurrentTab();
  finish();
}
