


let doc;

function createDocument()
{
  doc.body.innerHTML = '<h1>Sidebar state test</h1>';
  doc.title = "Sidebar State Test";

  InspectorUI.openInspectorUI();

  
  InspectorUI.currentInspector.once("sidebaractivated-ruleview", inspectorRuleViewOpened);

  InspectorUI.sidebar.show();
  InspectorUI.sidebar.activatePanel("ruleview");
}

function inspectorRuleViewOpened()
{
  is(InspectorUI.sidebar.activePanel, "ruleview", "Rule View is selected by default");

  
  InspectorUI.sidebar.activatePanel("computedview");

  Services.obs.addObserver(inspectorClosed,
    InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);
  InspectorUI.closeInspectorUI();
}

function inspectorClosed()
{
  
  Services.obs.removeObserver(inspectorClosed,
    InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED);

  Services.obs.addObserver(computedViewPopulated,
    "StyleInspector-populated", false);

  InspectorUI.openInspectorUI();
}

function computedViewPopulated()
{
  Services.obs.removeObserver(computedViewPopulated,
    "StyleInspector-populated");
  is(InspectorUI.sidebar.activePanel, "computedview", "Computed view is selected by default.");

  finishTest();
}


function finishTest()
{
  InspectorUI.closeInspectorUI();
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

