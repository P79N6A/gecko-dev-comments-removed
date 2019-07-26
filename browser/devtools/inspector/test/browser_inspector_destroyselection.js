


function test()
{
  waitForExplicitFinish();
  

  let node, iframe, inspector;

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    waitForFocus(setupTest, content);
  }, true);

  content.location = "http://mochi.test:8888/browser/browser/devtools/inspector/test/browser_inspector_destroyselection.html";

  function setupTest()
  {
    iframe = content.document.querySelector("iframe");
    node = iframe.contentDocument.querySelector("span");
    openInspector(runTests);
  }

  function runTests(aInspector)
  {
    inspector = aInspector;
    inspector.selection.setNode(node);

    iframe.parentNode.removeChild(iframe);
    iframe = null;

    let tmp = {};
    Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm", tmp);
    let lh = new tmp.LayoutHelpers(window.content);
    ok(!lh.isNodeConnected(node), "Node considered as disconnected.");
    ok(!inspector.selection.isConnected(), "Selection considered as disconnected");

    finishUp();
  }

  function finishUp() {
    node = null;
    gBrowser.removeCurrentTab();
    finish();
  }
}

