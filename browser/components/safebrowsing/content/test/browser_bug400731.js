
var newBrowser

function test() {
  waitForExplicitFinish();
  
  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  newBrowser = gBrowser.getBrowserForTab(newTab);
  
  
  
  window.addEventListener("DOMContentLoaded", testMalware, true);
  newBrowser.contentWindow.location = 'http://www.mozilla.com/firefox/its-an-attack.html';
}

function testMalware() {
  window.removeEventListener("DOMContentLoaded", testMalware, true);

  
  var el = newBrowser.contentDocument.getElementById("ignoreWarningButton");
  ok(el, "Ignore warning button should be present for malware");
  
  var style = newBrowser.contentWindow.getComputedStyle(el, null);
  is(style.display, "-moz-box", "Ignore Warning button should be display:-moz-box for malware");
  
  
  window.addEventListener("DOMContentLoaded", testPhishing, true);
  newBrowser.contentWindow.location = 'http://www.mozilla.com/firefox/its-a-trap.html';
}

function testPhishing() {
  window.removeEventListener("DOMContentLoaded", testPhishing, true);
  
  var el = newBrowser.contentDocument.getElementById("ignoreWarningButton");
  ok(el, "Ignore warning button should be present for phishing");
  
  var style = newBrowser.contentWindow.getComputedStyle(el, null);
  is(style.display, "-moz-box", "Ignore Warning button should be display:-moz-box for phishing");
  
  gBrowser.removeCurrentTab();
  finish();
}
