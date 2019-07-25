


let doc;

function createDocument()
{
  doc.body.innerHTML = '<h1>Sidebar state test</h1>';
  doc.title = "Sidebar State Test";

  
  Services.obs.addObserver(inspectorRuleViewOpened,
    InspectorUI.INSPECTOR_NOTIFICATIONS.RULEVIEWREADY, false);

  InspectorUI.openInspectorUI();
  InspectorUI.showSidebar();
}

function inspectorRuleViewOpened()
{
  Services.obs.removeObserver(inspectorRuleViewOpened,
    InspectorUI.INSPECTOR_NOTIFICATIONS.RULEVIEWREADY);
  is(InspectorUI.activeSidebarPanel, "ruleview", "Rule View is selected by default");

  
  InspectorUI.activateSidebarPanel("styleinspector");

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
  is(InspectorUI.activeSidebarPanel, "styleinspector", "Computed view is selected by default.");

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

