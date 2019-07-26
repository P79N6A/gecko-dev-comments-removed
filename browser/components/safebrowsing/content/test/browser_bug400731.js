

function test() {
  waitForExplicitFinish();
  
  gBrowser.selectedTab = gBrowser.addTab();
  
  
  
  
  
  gBrowser.addTabsProgressListener({
    onLocationChange: function(aTab, aWebProgress, aRequest, aLocation, aFlags) {
      if (aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_ERROR_PAGE) {
        gBrowser.removeTabsProgressListener(this);
        window.addEventListener("DOMContentLoaded", testMalware, true);
      }
    }
  });
  content.location = "http://www.mozilla.org/firefox/its-an-attack.html";
}

function testMalware() {
  window.removeEventListener("DOMContentLoaded", testMalware, true);

  
  var el = content.document.getElementById("ignoreWarningButton");
  ok(el, "Ignore warning button should be present for malware");
  
  var style = content.getComputedStyle(el, null);
  is(style.display, "inline-block", "Ignore Warning button should be display:inline-block for malware");
  
  
  window.addEventListener("DOMContentLoaded", testPhishing, true);
  content.location = "http://www.mozilla.org/firefox/its-a-trap.html";
}

function testPhishing() {
  window.removeEventListener("DOMContentLoaded", testPhishing, true);
  
  var el = content.document.getElementById("ignoreWarningButton");
  ok(el, "Ignore warning button should be present for phishing");
  
  var style = content.getComputedStyle(el, null);
  is(style.display, "inline-block", "Ignore Warning button should be display:inline-block for phishing");
  
  gBrowser.removeCurrentTab();
  finish();
}
