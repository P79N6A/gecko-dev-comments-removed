












































let div;
let tab1;

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

function inspectorTabOpen1()
{
  Services.obs.addObserver(inspectorUIOpen1,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.openInspectorUI();
}

function inspectorUIOpen1()
{
  Services.obs.removeObserver(inspectorUIOpen1,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);

  
  div = content.document.getElementsByTagName("div")[0];
  InspectorUI.inspectNode(div);

  
  waitForRuleView(ruleViewOpened1);

  InspectorUI.showSidebar();
  InspectorUI.ruleButton.click();
}

function ruleViewOpened1()
{
  let prop = InspectorUI.ruleView._elementStyle.rules[0].textProps[0];
  is(prop.name, "background-color", "First prop is the background color prop.");
  prop.setEnabled(false);

  
  tab2 = gBrowser.addTab();
  gBrowser.selectedTab = tab2;

  gBrowser.selectedBrowser.addEventListener("load", function(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, arguments.callee,
                                                 true);
    waitForFocus(inspectorTabOpen2, content);
  }, true);
  content.location = "data:text/html,<p>tab 2: the inspector should close now";
}

function inspectorTabOpen2()
{
  
  executeSoon(function() {
    Services.obs.addObserver(inspectorFocusTab1,
      InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
    gBrowser.removeCurrentTab();
    gBrowser.selectedTab = tab1;
  });
}

function inspectorFocusTab1()
{
  Services.obs.removeObserver(inspectorFocusTab1,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);

  
  waitForRuleView(ruleViewOpened2);
}

function ruleViewOpened2()
{
  let prop = InspectorUI.ruleView._elementStyle.rules[0].textProps[0];
  is(prop.name, "background-color", "First prop is the background color prop.");
  ok(!prop.enabled, "First prop should be disabled.");

  gBrowser.removeCurrentTab();
  InspectorUI.closeInspectorUI();
  finish();
}

function test()
{
  waitForExplicitFinish();
  ignoreAllUncaughtExceptions();

  tab1 = gBrowser.addTab();
  gBrowser.selectedTab = tab1;
  gBrowser.selectedBrowser.addEventListener("load", function(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, arguments.callee,
      true);
    waitForFocus(inspectorTabOpen1, content);
  }, true);

  content.location = "data:text/html,<p>tab switching tests for inspector" +
    '<div style="background-color: green;">tab 1</div>';
}

