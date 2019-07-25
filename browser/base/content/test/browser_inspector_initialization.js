






































let doc;

function startInspectorTests()
{
  ok(InspectorUI, "InspectorUI variable exists");
  Services.obs.addObserver(runInspectorTests, "inspector-opened", false);
  InspectorUI.toggleInspectorUI();
}

function runInspectorTests()
{
  Services.obs.removeObserver(runInspectorTests, "inspector-opened", false);
  Services.obs.addObserver(finishInspectorTests, "inspector-closed", false);
  let iframe = document.getElementById("inspector-tree-iframe");
  is(InspectorUI.treeIFrame, iframe, "Inspector IFrame matches");
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.isTreePanelOpen, "Inspector Tree Panel is open");
  InspectorUI.closeInspectorUI();
}

function finishInspectorTests()
{
  Services.obs.removeObserver(finishInspectorTests, "inspector-closed", false);
  ok(!InspectorUI.isTreePanelOpen, "Inspector Tree Panel is closed");
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  gBrowser.removeCurrentTab();
  finish();
}

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    doc = content.document;
    waitForFocus(startInspectorTests, content);
  }, true);
  
  content.location = "data:text/html,basic tests for inspector";
}

