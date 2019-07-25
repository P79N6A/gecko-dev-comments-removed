


let doc;

function createDocument() {
  doc.body.innerHTML = '<h1>highlighter autohide test</h1>';

  InspectorUI.openInspectorUI(doc.querySelector("h1"));

  
  InspectorUI.currentInspector.once("sidebaractivated-ruleview", inspectorRuleViewOpened);

  InspectorUI.sidebar.show();
  InspectorUI.sidebar.activatePanel("ruleview");
}

function inspectorRuleViewOpened() {
  let deck = InspectorUI.sidebar._deck;

  EventUtils.synthesizeMouse(InspectorUI.highlighter.highlighterContainer, 2, 2, {type: "mousemove"}, window);

  executeSoon(function() {
    ok(!InspectorUI.highlighter.hidden, "Outline visible (1)");

    EventUtils.synthesizeMouse(deck, 10, 2, {type: "mouseover"}, window);

    executeSoon(function() {
      ok(InspectorUI.highlighter.hidden, "Outline not visible");

      EventUtils.synthesizeMouse(deck, -10, 2, {type: "mouseover"}, window);

      executeSoon(function() {
        ok(!InspectorUI.highlighter.hidden, "Outline visible (2)");
        finishTest();
      });
    });
  });
}

function finishTest() {
  InspectorUI.closeInspectorUI();
  gBrowser.removeCurrentTab();
  finish();
}

function test() {
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    doc = content.document;
    waitForFocus(createDocument, content);
  }, true);

  content.location = "data:text/html,basic tests for highlighter";
}


