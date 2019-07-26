




let doc;
let testDiv;

function createDocument()
{
  doc.body.innerHTML = '<div id="testdiv">Test div!</div>';
  doc.title = "Inspector Change Test";
  startInspectorTests();
}


function getInspectorProp(aName)
{
  for each (let view in computedViewTree().propertyViews) {
    if (view.name == aName) {
      return view;
    }
  }
  return null;
}

function startInspectorTests()
{
  ok(InspectorUI, "InspectorUI variable exists");
  Services.obs.addObserver(runInspectorTests,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.toggleInspectorUI();
}

function runInspectorTests()
{
  Services.obs.removeObserver(runInspectorTests, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED);
  testDiv = doc.getElementById("testdiv");

  testDiv.style.fontSize = "10px";

  InspectorUI.inspectNode(testDiv);
  InspectorUI.stopInspecting();

  
  Services.obs.addObserver(stylePanelTests, "StyleInspector-populated", false);

  InspectorUI.sidebar.show();
  InspectorUI.sidebar.activatePanel("computedview");
}

function stylePanelTests()
{
  Services.obs.removeObserver(stylePanelTests, "StyleInspector-populated");

  ok(InspectorUI.sidebar.visible, "Inspector Sidebar is open");
  ok(computedViewTree(), "Style Panel has a cssHtmlTree");

  let propView = getInspectorProp("font-size");
  is(propView.value, "10px", "Style inspector should be showing the correct font size.");

  Services.obs.addObserver(stylePanelAfterChange, "StyleInspector-populated", false);

  testDiv.style.fontSize = "15px";
  InspectorUI.nodeChanged();
}

function stylePanelAfterChange()
{
  Services.obs.removeObserver(stylePanelAfterChange, "StyleInspector-populated");

  let propView = getInspectorProp("font-size");
  is(propView.value, "15px", "Style inspector should be showing the new font size.");

  stylePanelNotActive();
}

function stylePanelNotActive()
{
  
  InspectorUI.sidebar.activatePanel("ruleview");

  executeSoon(function() {
    Services.obs.addObserver(stylePanelAfterSwitch, "StyleInspector-populated", false);
    testDiv.style.fontSize = "20px";
    InspectorUI.nodeChanged();
    InspectorUI.sidebar.activatePanel("computedview");
  });
}

function stylePanelAfterSwitch()
{
  Services.obs.removeObserver(stylePanelAfterSwitch, "StyleInspector-populated");

  let propView = getInspectorProp("font-size");
  is(propView.value, "20px", "Style inspector should be showing the newest font size.");

  Services.obs.addObserver(finishTest, InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);
  executeSoon(function() {
    InspectorUI.closeInspectorUI(true);
  });
}

function finishTest()
{
  Services.obs.removeObserver(finishTest,
    InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED);
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
    waitForFocus(createDocument, content);
  }, true);

  content.location = "data:text/html,basic tests for inspector";
}

