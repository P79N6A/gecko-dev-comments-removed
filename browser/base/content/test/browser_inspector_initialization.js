






































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
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.isTreePanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  ok(InspectorUI.isDOMPanelOpen, "Inspector DOM Panel is open");
  InspectorUI.toggleInspectorUI();
}

function finishInspectorTests()
{
  Services.obs.removeObserver(finishInspectorTests, "inspector-closed", false);
  ok(!InspectorUI.isDOMPanelOpen, "Inspector DOM Panel is closed");
  ok(!InspectorUI.isStylePanelOpen, "Inspector Style Panel is closed");
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

