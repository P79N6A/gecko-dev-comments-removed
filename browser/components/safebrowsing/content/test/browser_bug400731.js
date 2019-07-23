
var newBrowser

function test() {
  waitForExplicitFinish();
  
  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  newBrowser = gBrowser.getBrowserForTab(newTab);
  
  
  
  newBrowser.contentWindow.location = 'http://www.mozilla.com/firefox/its-an-attack.html';
  window.setTimeout(testMalware, 2000);
}

function testMalware() {
  
  var el = newBrowser.contentDocument.getElementById("ignoreWarningButton");
  ok(el, "Ignore warning button should be present (but hidden) for malware");
  
  var style = newBrowser.contentWindow.getComputedStyle(el, null);
  is(style.display, "none", "Ignore Warning button should be display:none for malware");
  
  
  newBrowser.contentWindow.location = 'http://www.mozilla.com/firefox/its-a-trap.html';
  window.setTimeout(testPhishing, 2000);
}

function testPhishing() {
  var el = newBrowser.contentDocument.getElementById("ignoreWarningButton");
  ok(el, "Ignore warning button should be present for phishing");
  
  var style = newBrowser.contentWindow.getComputedStyle(el, null);
  is(style.display, "-moz-box", "Ignore Warning button should be display:-moz-box for phishing");
  
  gBrowser.removeCurrentTab();
  finish();
}
