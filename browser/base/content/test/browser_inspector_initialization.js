






































let doc;

function startInspectorTests()
{
  ok(InspectorUI, "InspectorUI variable exists");
  document.addEventListener("popupshown", runInspectorTests, false);
  InspectorUI.toggleInspectorUI();
}

function runInspectorTests(evt)
{
  if (evt.target.id != "inspector-panel")
    return true;
  document.removeEventListener("popupshown", runInspectorTests, false);
  document.addEventListener("popuphidden", finishInspectorTests, false);
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.isPanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  todo(InspectorUI.isDOMPanelOpen, "Inspector DOM Panel is open");
  InspectorUI.toggleInspectorUI();
}

function finishInspectorTests(evt)
{
  if (evt.target.id != "inspector-style-panel")
    return true;
  document.removeEventListener("popuphidden", finishInspectorTests, false);
  ok(!InspectorUI.isDOMPanelOpen, "Inspector DOM Panel is closed");
  ok(!InspectorUI.isStylePanelOpen, "Inspector Style Panel is closed");
  ok(!InspectorUI.isPanelOpen, "Inspector Tree Panel is closed");
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

