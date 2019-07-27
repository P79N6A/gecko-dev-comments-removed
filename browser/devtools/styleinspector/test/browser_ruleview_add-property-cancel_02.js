



"use strict";



let TEST_URL = 'url("' + TEST_URL_ROOT + 'doc_test_image.png")';
let PAGE_CONTENT = [
  '<style type="text/css">',
  '  #testid {',
  '    background-color: blue;',
  '  }',
  '</style>',
  '<div id="testid">Styled Node</div>'
].join("\n");

add_task(function*() {
  yield addTab("data:text/html;charset=utf-8,test rule view user changes");

  info("Creating the test document");
  content.document.body.innerHTML = PAGE_CONTENT;

  info("Opening the rule-view");
  let {toolbox, inspector, view} = yield openRuleView();

  info("Selecting the test element");
  yield selectNode("#testid", inspector);

  info("Test creating a new property and escaping");

  let elementRuleEditor = getRuleViewRuleEditor(view, 1);

  info("Focusing a new property name in the rule-view");
  let editor = yield focusEditableField(view, elementRuleEditor.closeBrace);

  is(inplaceEditor(elementRuleEditor.newPropSpan), editor, "The new property editor got focused.");

  info("Entering a value in the property name editor");
  editor.input.value = "color";

  info("Pressing return to commit and focus the new value field");
  let onValueFocus = once(elementRuleEditor.element, "focus", true);
  let onRuleViewChanged = view.once("ruleview-changed");
  EventUtils.synthesizeKey("VK_RETURN", {}, view.doc.defaultView);
  yield onValueFocus;
  yield onRuleViewChanged;

  
  editor = inplaceEditor(view.doc.activeElement);
  let textProp = elementRuleEditor.rule.textProps[1];

  is(elementRuleEditor.rule.textProps.length,  2, "Created a new text property.");
  is(elementRuleEditor.propertyList.children.length, 2, "Created a property editor.");
  is(editor, inplaceEditor(textProp.editor.valueSpan), "Editing the value span now.");

  info("Entering a property value");
  editor.input.value = "red";

  info("Escaping out of the field");
  onRuleViewChanged = view.once("ruleview-changed");
  EventUtils.synthesizeKey("VK_ESCAPE", {}, view.doc.defaultView);
  yield onRuleViewChanged;

  info("Checking that the previous field is focused");
  let focusedElement = inplaceEditor(elementRuleEditor.rule.textProps[0].editor.valueSpan).input;
  is(focusedElement, focusedElement.ownerDocument.activeElement, "Correct element has focus");

  onRuleViewChanged = view.once("ruleview-changed");
  EventUtils.synthesizeKey("VK_ESCAPE", {}, view.doc.defaultView);
  yield onRuleViewChanged;

  is(elementRuleEditor.rule.textProps.length,  1, "Removed the new text property.");
  is(elementRuleEditor.propertyList.children.length, 1, "Removed the property editor.");
});
