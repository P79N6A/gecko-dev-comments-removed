






let doc;
let stylePanel;

function waitForRuleView(aCallback)
{
  if (InspectorUI.ruleView) {
    aCallback();
    return;
  }

  let ruleViewFrame = InspectorUI.getToolIframe(InspectorUI.ruleViewObject);
  ruleViewFrame.addEventListener("load", function(evt) {
    ruleViewFrame.removeEventListener(evt.type, arguments.callee, true);
    executeSoon(function() {
      aCallback();
    });
  }, true);
}

function waitForEditorFocus(aParent, aCallback)
{
  aParent.addEventListener("focus", function onFocus(evt) {
    if (evt.target.inplaceEditor) {
      aParent.removeEventListener("focus", onFocus, true);
      let editor = evt.target.inplaceEditor;
      executeSoon(function() {
        aCallback(editor);
      });
    }
  }, true);
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

    InspectorUI.showSidebar();
    InspectorUI.ruleButton.click();

    testFocus();
  }, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.openInspectorUI();
}

function testFocus()
{
  let ruleViewFrame = InspectorUI.getToolIframe(InspectorUI.ruleViewObject);
  let brace = ruleViewFrame.contentDocument.querySelectorAll(".ruleview-ruleclose")[0];
  waitForEditorFocus(brace.parentNode, function onNewElement(aEditor) {
    aEditor.input.value = "color";
    waitForEditorFocus(brace.parentNode, function onEditingValue(aEditor) {
      
      ok(true, "We got focus.");
      aEditor.input.value = "green";

      
      
      waitForEditorFocus(brace.parentNode, function onNewEditor(aEditor) {
        aEditor.input.blur();
        finishTest();
      });
      EventUtils.sendKey("return");
    });
    EventUtils.sendKey("return");
  });

  brace.focus();
}

function finishUp()
{
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
