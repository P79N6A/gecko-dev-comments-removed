





































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-bug-585956-console-trace.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", tabLoaded, true);
}

function tabLoaded() {
  browser.removeEventListener("load", tabLoaded, true);

  openConsole();

  browser.addEventListener("load", tabReloaded, true);
  content.location.reload();
}

function tabReloaded() {
  browser.removeEventListener("load", tabReloaded, true);

  
  let stacktrace = [
    { filename: TEST_URI, lineNumber: 9, functionName: null, language: 2 },
    { filename: TEST_URI, lineNumber: 14, functionName: "foobar585956b", language: 2 },
    { filename: TEST_URI, lineNumber: 18, functionName: "foobar585956a", language: 2 },
    { filename: TEST_URI, lineNumber: 21, functionName: null, language: 2 }
  ];

  let hudId = HUDService.getHudIdByWindow(content);
  let HUD = HUDService.hudReferences[hudId];

  let node = HUD.outputNode.querySelector(".hud-log");
  ok(node, "found trace log node");
  ok(node._stacktrace, "found stacktrace object");
  is(node._stacktrace.toSource(), stacktrace.toSource(), "stacktrace is correct");
  isnot(node.textContent.indexOf("bug-585956"), -1, "found file name");

  finishTest();
}
