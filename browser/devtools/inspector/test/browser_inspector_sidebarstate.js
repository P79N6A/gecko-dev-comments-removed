


let doc;
let inspector;

function createDocument()
{
  doc.body.innerHTML = '<h1>Sidebar state test</h1>';
  doc.title = "Sidebar State Test";

  openInspector(function(panel) {
    inspector = panel;
    inspector.sidebar.select("ruleview");
    inspectorRuleViewOpened();
  });
}

function inspectorRuleViewOpened()
{
  is(inspector.sidebar.getCurrentTabID(), "ruleview", "Rule View is selected by default");

  
  inspector.sidebar.select("computedview");

  gDevTools.once("toolbox-destroyed", inspectorClosed);
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.getToolbox(target).destroy();
}

function inspectorClosed()
{
  openInspector(function(panel) {
    inspector = panel;
    if (inspector.sidebar.getCurrentTabID()) {
      
      testNewDefaultTab();
    } else {
      
      inspector.sidebar.once("select", testNewDefaultTab);
    }
  });
}

function testNewDefaultTab()
{
  is(inspector.sidebar.getCurrentTabID(), "computedview", "Computed view is selected by default.");

  finishTest();
}


function finishTest()
{
  doc = inspector = null;
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

  content.location = "data:text/html;charset=utf-8,browser_inspector_sidebarstate.js";
}
