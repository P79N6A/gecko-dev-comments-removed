






let doc;
let ruleWindow;
let ruleView;
let inspector;
let originalValue = "#00F";








let testData = [
  {value: "red", commitKey: "VK_ESCAPE", modifiers: {}, expected: originalValue},
  {value: "red", commitKey: "VK_RETURN", modifiers: {}, expected: "red"},
  {value: "blue", commitKey: "VK_TAB", modifiers: {shiftKey: true}, expected: "blue"}
];

function startTests()
{
  let style = '' +
    '#testid {' +
    '  color: ' + originalValue + ';' +
    '}';

  let styleNode = addStyle(doc, style);
  doc.body.innerHTML = '<div id="testid">Styled Node</div>';
  let testElement = doc.getElementById("testid");

  openRuleView((aInspector, aRuleView) => {
    inspector = aInspector;
    ruleView = aRuleView;
    ruleWindow = aRuleView.doc.defaultView;
    inspector.selection.setNode(testElement);
    inspector.once("inspector-updated", () => runTestData(0));
  });
}

function runTestData(index)
{
  if (index === testData.length) {
    finishTest();
    return;
  }

  let idRuleEditor = ruleView.element.children[1]._ruleEditor;
  let propEditor = idRuleEditor.rule.textProps[0].editor;
  waitForEditorFocus(propEditor.element, function(aEditor) {
    is(inplaceEditor(propEditor.valueSpan), aEditor, "Focused editor should be the value span.");

    for (let ch of testData[index].value) {
      EventUtils.sendChar(ch, ruleWindow);
    }
    EventUtils.synthesizeKey(testData[index].commitKey, testData[index].modifiers);

    is(propEditor.valueSpan.innerHTML, testData[index].expected);

    runTestData(index + 1);
  });

  EventUtils.synthesizeMouse(propEditor.valueSpan, 1, 1, {}, ruleWindow);
}

function finishTest()
{
  inspector = ruleWindow = ruleView = null;
  doc = null;
  gBrowser.removeCurrentTab();
  finish();
}

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function escapePropertyChange_load(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, escapePropertyChange_load, true);
    doc = content.document;
    waitForFocus(startTests, content);
  }, true);

  content.location = "data:text/html,test escaping property change reverts back to original value";
}
