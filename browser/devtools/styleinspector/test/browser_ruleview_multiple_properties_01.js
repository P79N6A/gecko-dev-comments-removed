



"use strict";




add_task(function*() {
  yield addTab("data:text/html;charset=utf-8,test rule view user changes");
  content.document.body.innerHTML = "<h1>Testing Multiple Properties</h1>";
  let {toolbox, inspector, view} = yield openRuleView();

  info("Creating the test element");
  let newElement = content.document.createElement("div");
  newElement.textContent = "Test Element";
  content.document.body.appendChild(newElement);
  yield selectNode("div", inspector);
  let ruleEditor = getRuleViewRuleEditor(view, 0);

  yield testCreateNewMulti(inspector, ruleEditor);
});

function* testCreateNewMulti(inspector, ruleEditor) {
  let onMutation = inspector.once("markupmutation");
  yield createNewRuleViewProperty(ruleEditor,
    "color:blue;background : orange   ; text-align:center; border-color: green;");
  yield onMutation;

  is(ruleEditor.rule.textProps.length, 4, "Should have created a new text property.");
  is(ruleEditor.propertyList.children.length, 5, "Should have created a new property editor.");

  is(ruleEditor.rule.textProps[0].name, "color", "Should have correct property name");
  is(ruleEditor.rule.textProps[0].value, "blue", "Should have correct property value");

  is(ruleEditor.rule.textProps[1].name, "background", "Should have correct property name");
  is(ruleEditor.rule.textProps[1].value, "orange", "Should have correct property value");

  is(ruleEditor.rule.textProps[2].name, "text-align", "Should have correct property name");
  is(ruleEditor.rule.textProps[2].value, "center", "Should have correct property value");

  is(ruleEditor.rule.textProps[3].name, "border-color", "Should have correct property name");
  is(ruleEditor.rule.textProps[3].value, "green", "Should have correct property value");
}
