

function test() {
  waitForExplicitFinish();
  
  gBrowser.selectedTab = gBrowser.addTab();
  
  
  
  window.addEventListener("DOMContentLoaded", testMalware, true);
  content.location = "http://www.mozilla.com/firefox/its-an-attack.html";
}

function testMalware() {
  window.removeEventListener("DOMContentLoaded", testMalware, true);

  
  var el = content.document.getElementById("ignoreWarningButton");
  ok(el, "Ignore warning button should be present for malware");
  
  var style = content.getComputedStyle(el, null);
  is(style.display, "-moz-box", "Ignore Warning button should be display:-moz-box for malware");
  
  
  window.addEventListener("DOMContentLoaded", testPhishing, true);
  content.location = "http://www.mozilla.com/firefox/its-a-trap.html";
}

function testPhishing() {
  window.removeEventListener("DOMContentLoaded", testPhishing, true);
  
  var el = content.document.getElementById("ignoreWarningButton");
  ok(el, "Ignore warning button should be present for phishing");
  
  var style = content.getComputedStyle(el, null);
  is(style.display, "-moz-box", "Ignore Warning button should be display:-moz-box for phishing");
  
  gBrowser.removeCurrentTab();
  finish();
}
