var newBrowser

function test() {
  waitForExplicitFinish();
  
  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  newBrowser = gBrowser.getBrowserForTab(newTab);
  
  
  newBrowser.contentWindow.location = 'https://nocert.example.com/';
  
  
  window.setTimeout(testBrokenCert, 2000);
}

function testBrokenCert() {
  
  
  ok(/^about:certerror/.test(gBrowser.contentWindow.document.documentURI), "Broken page should go to about:certerror, not about:neterror");
  
  
  var expertDiv = gBrowser.contentWindow.document.getElementById("expertContent");
  ok(expertDiv, "Expert content div should exist");
  ok(expertDiv.hasAttribute("collapsed"), "Expert content should be collapsed by default");
  
  
  Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch)
                                          .setBoolPref("browser.xul.error_pages.expert_bad_cert", true);
  
  newBrowser.reload();
  window.setTimeout(testExpertPref, 2000);
}

function testExpertPref() {
  
  var expertDiv = gBrowser.contentWindow.document.getElementById("expertContent");
  ok(!expertDiv.hasAttribute("collapsed"), "Expert content should not be collapsed with the expert mode pref set");
  
  
  gBrowser.removeCurrentTab();
  finish();
}
