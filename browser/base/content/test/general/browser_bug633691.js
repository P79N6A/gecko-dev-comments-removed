



function test() {
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  
  gBrowser.selectedTab.linkedBrowser.addEventListener("load", testIframeCert, true);
  content.location = "data:text/html,<iframe width='700' height='700' src='about:certerror'></iframe>";
}

function testIframeCert(e) {
  if (e.target.location.href == "about:blank") {
    return;
  }
  gBrowser.selectedTab.linkedBrowser.removeEventListener("load", testIframeCert, true);
  
  var doc = gBrowser.contentDocument.getElementsByTagName('iframe')[0].contentDocument;
  var eC = doc.getElementById("expertContent");
  ok(eC, "Expert content should exist")
  ok(eC.hasAttribute("hidden"), "Expert content should be hidded by default");

  
  gBrowser.removeCurrentTab();
  
  finish();
}
