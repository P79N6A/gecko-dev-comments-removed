






let tempScope = {};
Cu.import("resource:///modules/devtools/CssRuleView.jsm", tempScope);
let inplaceEditor = tempScope._getInplaceEditorForSpan;
let doc;
let stylePanel;

function waitForRuleView(aCallback)
{
  InspectorUI.currentInspector.once("sidebaractivated-ruleview", aCallback);
}

function openRuleView()
{
  Services.obs.addObserver(function onOpened() {
    Services.obs.removeObserver(onOpened,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);

    
    let node = content.document.getElementsByTagName("h1")[0];
    InspectorUI.inspectNode(node);
    InspectorUI.stopInspecting();

    
    waitForRuleView(testFocus);

    InspectorUI.sidebar.show();
    InspectorUI.sidebar.activatePanel("ruleview");

    testFocus();
  }, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.openInspectorUI();
}

function testFocus()
{
  let ruleViewFrame = InspectorUI.sidebar._tools["ruleview"].frame;
  let brace = ruleViewFrame.contentDocument.querySelectorAll(".ruleview-ruleclose")[0];
  waitForEditorFocus(brace.parentNode, function onNewElement(aEditor) {
    aEditor.input.value = "color";
    waitForEditorFocus(brace.parentNode, function onEditingValue(aEditor) {
      
      ok(true, "We got focus.");
      aEditor.input.value = "green";

      
      
      waitForEditorFocus(brace.parentNode, function onNewEditor(aEditor) {
        aEditor.input.blur();
        finishUp();
      });
      EventUtils.sendKey("return");
    });
    EventUtils.sendKey("return");
  });

  brace.click();
}

function finishUp()
{
  InspectorUI.sidebar.hide();
  InspectorUI.closeInspectorUI();
  doc = stylePanel = null;
  gBrowser.removeCurrentTab();
  finish();
}

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, arguments.callee, true);
    doc = content.document;
    doc.title = "Rule View Test";
    waitForFocus(openRuleView, content);
  }, true);

  content.location = "data:text/html,<h1>Some header text</h1>";
}
