






































function startInspectorTests()
{
  ok(InspectorUI, "InspectorUI variable exists");
  Services.obs.addObserver(runInspectorTests,
    INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.toggleInspectorUI();
}

function runInspectorTests()
{
  Services.obs.removeObserver(runInspectorTests,
    INSPECTOR_NOTIFICATIONS.OPENED, false);
  Services.obs.addObserver(finishInspectorTests,
    INSPECTOR_NOTIFICATIONS.CLOSED, false);

  let iframe = document.getElementById("inspector-tree-iframe");
  is(InspectorUI.treeIFrame, iframe, "Inspector IFrame matches");
  ok(InspectorUI.inspecting, "Inspector is inspecting");
  ok(InspectorUI.isTreePanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.highlighter, "Highlighter is up");

  executeSoon(function() {
    InspectorUI.closeInspectorUI();
  });
}

function finishInspectorTests()
{
  Services.obs.removeObserver(finishInspectorTests,
    INSPECTOR_NOTIFICATIONS.CLOSED, false);

  ok(!InspectorUI.highlighter, "Highlighter is gone");
  ok(!InspectorUI.isTreePanelOpen, "Inspector Tree Panel is closed");
  ok(!InspectorUI.inspecting, "Inspector is not inspecting");

  gBrowser.removeCurrentTab();
  finish();
}

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    waitForFocus(startInspectorTests, content);
  }, true);

  content.location = "data:text/html,basic tests for inspector";
}

