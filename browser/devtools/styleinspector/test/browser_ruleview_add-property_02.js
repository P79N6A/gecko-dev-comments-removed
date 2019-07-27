



"use strict";




add_task(function*() {
  yield addTab("data:text/html;charset=utf-8,browser_ruleview_ui.js");
  let {toolbox, inspector, view} = yield openRuleView();

  info("Creating the test document");
  let style = "" +
    "#testid {" +
    "  background-color: blue;" +
    "}" +
    ".testclass, .unmatched {" +
    "  background-color: green;" +
    "}";
  let styleNode = addStyle(content.document, style);
  content.document.body.innerHTML = "<div id='testid' class='testclass'>Styled Node</div>" +
                                    "<div id='testid2'>Styled Node</div>";

  yield testCreateNew(inspector, view);
});

function* testCreateNew(inspector, ruleView) {
  
  let elementRuleEditor = getRuleViewRuleEditor(ruleView, 0);
  let editor = yield focusEditableField(elementRuleEditor.closeBrace);

  is(inplaceEditor(elementRuleEditor.newPropSpan), editor,
    "Next focused editor should be the new property editor.");

  let input = editor.input;

  ok(input.selectionStart === 0 && input.selectionEnd === input.value.length,
    "Editor contents are selected.");

  
  EventUtils.synthesizeMouse(input, 1, 1, {}, ruleView.doc.defaultView);
  input.select();

  info("Entering the property name");
  input.value = "background-color";

  info("Pressing RETURN and waiting for the value field focus");
  let onFocus = once(elementRuleEditor.element, "focus", true);
  EventUtils.sendKey("return", ruleView.doc.defaultView);
  yield onFocus;
  yield elementRuleEditor.rule._applyingModifications;

  editor = inplaceEditor(ruleView.doc.activeElement);

  is(elementRuleEditor.rule.textProps.length,  1, "Should have created a new text property.");
  is(elementRuleEditor.propertyList.children.length, 1, "Should have created a property editor.");
  let textProp = elementRuleEditor.rule.textProps[0];
  is(editor, inplaceEditor(textProp.editor.valueSpan), "Should be editing the value span now.");

  let onMutated = inspector.once("markupmutation");
  editor.input.value = "purple";
  let onBlur = once(editor.input, "blur");
  EventUtils.sendKey("return", ruleView.doc.defaultView);
  yield onBlur;
  yield onMutated;

  is(textProp.value, "purple", "Text prop should have been changed.");
}
