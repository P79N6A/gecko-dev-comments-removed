var newBrowser

function test() {
  waitForExplicitFinish();
  
  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  newBrowser = gBrowser.getBrowserForTab(newTab);
  
  
  window.addEventListener("DOMContentLoaded", testBrokenCert, true);
  newBrowser.contentWindow.location = 'https://nocert.example.com/';
}

function testBrokenCert() {
  window.removeEventListener("DOMContentLoaded", testBrokenCert, true);
  
  
  ok(/^about:certerror/.test(gBrowser.contentWindow.document.documentURI), "Broken page should go to about:certerror, not about:neterror");
  
  
  var expertDiv = gBrowser.contentWindow.document.getElementById("expertContent");
  ok(expertDiv, "Expert content div should exist");
  ok(expertDiv.hasAttribute("collapsed"), "Expert content should be collapsed by default");
  
  
  Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch)
                                          .setBoolPref("browser.xul.error_pages.expert_bad_cert", true);
  
  window.addEventListener("DOMContentLoaded", testExpertPref, true);
  newBrowser.reload();
}

function testExpertPref() {
  
  window.removeEventListener("DOMContentLoaded", testExpertPref, true);
  var expertDiv = gBrowser.contentWindow.document.getElementById("expertContent");
  var technicalDiv = gBrowser.contentWindow.document.getElementById("technicalContent");
  ok(!expertDiv.hasAttribute("collapsed"), "Expert content should not be collapsed with the expert mode pref set");
  ok(!technicalDiv.hasAttribute("collapsed"), "Technical content should not be collapsed with the expert mode pref set");
  
  
  gBrowser.removeCurrentTab();
  finish();
}
