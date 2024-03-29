



"use strict";



let PAGE_CONTENT = [
  '<style type="text/css">',
  '  .testclass {',
  '    text-align: center;',
  '  }',
  '</style>',
  '<div id="testid" class="testclass">Styled Node</div>',
  '<span>This is a span</span>',
].join("\n");

add_task(function*() {
  yield addTab("data:text/html;charset=utf-8," + encodeURIComponent(PAGE_CONTENT));
  let {inspector, view} = yield openRuleView();

  info("Selecting the test element");
  yield selectNode("#testid", inspector);
  yield testEditSelector(view, "span");

  info("Selecting the modified element with the new rule");
  yield selectNode("span", inspector);
  yield checkModifiedElement(view, "span");
});

function* testEditSelector(view, name) {
  info("Test editing existing selector fields");

  let idRuleEditor = getRuleViewRuleEditor(view, 1);

  info("Focusing an existing selector name in the rule-view");
  let editor = yield focusEditableField(view, idRuleEditor.selectorText);

  is(inplaceEditor(idRuleEditor.selectorText), editor,
    "The selector editor got focused");

  info("Entering a new selector name and committing");
  editor.input.value = name;

  info("Waiting for rule view to update");
  let onRuleViewChanged = once(view, "ruleview-changed");

  info("Entering the commit key");
  EventUtils.synthesizeKey("VK_RETURN", {});
  yield onRuleViewChanged;

  is(view._elementStyle.rules.length, 2, "Should have 2 rules.");
  ok(getRuleViewRule(view, name), "Rule with " + name + " selector exists.");
  ok(getRuleViewRuleEditor(view, 1).element.getAttribute("unmatched"),
    "Rule with " + name + " does not match the current element.");
}

function* checkModifiedElement(view, name) {
  is(view._elementStyle.rules.length, 2, "Should have 2 rules.");
  ok(getRuleViewRule(view, name), "Rule with " + name + " selector exists.");
}
