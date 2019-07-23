function test() {
  waitForExplicitFinish();

  
  
  document.getElementById("sidebar").addEventListener("load", delayedOpenUrl, true);
  toggleSidebar("viewWebPanelsSidebar", true);
}

function delayedOpenUrl() {
  setTimeout(openPanelUrl, 100);
}

function openPanelUrl(event) {
  ok(!document.getElementById("sidebar-box").hidden, "Sidebar showing");

  var sidebar = document.getElementById("sidebar");
  var root = sidebar.contentDocument.documentElement;
  ok(root.nodeName != "parsererror", "Sidebar is well formed");

  sidebar.removeEventListener("load", delayedOpenUrl, true);
  
  sidebar.contentDocument.addEventListener("load", delayedRunTest, true);
  var url = 'data:text/html,<div%20id="test_bug409481">Content!</div>';
  sidebar.contentWindow.loadWebPanel(url);
}

function delayedRunTest() {
  setTimeout(runTest, 100);
}

function runTest(event) {
  var sidebar = document.getElementById("sidebar");
  sidebar.contentDocument.removeEventListener("load", delayedRunTest, true);

  var browser = sidebar.contentDocument.getElementById("web-panels-browser");
  var div = browser && browser.contentDocument.getElementById("test_bug409481");
  ok(div && div.textContent == "Content!", "Sidebar content loaded");

  toggleSidebar("viewWebPanelsSidebar");

  ok(document.getElementById("sidebar-box").hidden, "Sidebar successfully hidden");

  finish();
}
