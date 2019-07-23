function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();

  
  window.addEventListener("DOMContentLoaded", testBrokenCert, true);
  content.location = "https://nocert.example.com/";
}

function testBrokenCert() {
  window.removeEventListener("DOMContentLoaded", testBrokenCert, true);

  
  ok(/^about:certerror/.test(gBrowser.contentDocument.documentURI), "Broken page should go to about:certerror, not about:neterror");

  
  var expertDiv = gBrowser.contentDocument.getElementById("expertContent");
  ok(expertDiv, "Expert content div should exist");
  ok(expertDiv.hasAttribute("collapsed"), "Expert content should be collapsed by default");

  
  gPrefService.setBoolPref("browser.xul.error_pages.expert_bad_cert", true);

  window.addEventListener("DOMContentLoaded", testExpertPref, true);
  gBrowser.reload();
}

function testExpertPref() {
  window.removeEventListener("DOMContentLoaded", testExpertPref, true);
  var expertDiv = gBrowser.contentDocument.getElementById("expertContent");
  var technicalDiv = gBrowser.contentDocument.getElementById("technicalContent");
  ok(!expertDiv.hasAttribute("collapsed"), "Expert content should not be collapsed with the expert mode pref set");
  ok(!technicalDiv.hasAttribute("collapsed"), "Technical content should not be collapsed with the expert mode pref set");

  
  gBrowser.removeCurrentTab();
  gPrefService.clearUserPref("browser.xul.error_pages.expert_bad_cert");
  finish();
}
