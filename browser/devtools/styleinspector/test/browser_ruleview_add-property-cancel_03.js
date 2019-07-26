



"use strict";



let test = asyncTest(function*() {
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

  yield testCancelNew(inspector, view);
  yield testCancelNewOnEscape(inspector, view);
  yield inspector.once("inspector-updated");
});

function* testCancelNew(inspector, ruleView) {
  
  

  let elementRuleEditor = getRuleViewRuleEditor(ruleView, 0);
  let editor = yield focusEditableField(elementRuleEditor.closeBrace);

  is(inplaceEditor(elementRuleEditor.newPropSpan), editor,
    "Property editor is focused");

  let onBlur = once(editor.input, "blur");
  editor.input.blur();
  yield onBlur;

  ok(!elementRuleEditor.rule._applyingModifications, "Shouldn't have an outstanding modification request after a cancel.");
  is(elementRuleEditor.rule.textProps.length, 0, "Should have canceled creating a new text property.");
  ok(!elementRuleEditor.propertyList.hasChildNodes(), "Should not have any properties.");
}

function* testCancelNewOnEscape(inspector, ruleView) {
  
  

  let elementRuleEditor = getRuleViewRuleEditor(ruleView, 0);
  let editor = yield focusEditableField(elementRuleEditor.closeBrace);

  is(inplaceEditor(elementRuleEditor.newPropSpan), editor, "Next focused editor should be the new property editor.");
  for (let ch of "background") {
    EventUtils.sendChar(ch, ruleView.doc.defaultView);
  }

  let onBlur = once(editor.input, "blur");
  EventUtils.synthesizeKey("VK_ESCAPE", {});
  yield onBlur;

  ok(!elementRuleEditor.rule._applyingModifications, "Shouldn't have an outstanding modification request after a cancel.");
  is(elementRuleEditor.rule.textProps.length, 0, "Should have canceled creating a new text property.");
  ok(!elementRuleEditor.propertyList.hasChildNodes(), "Should not have any properties.");
}
