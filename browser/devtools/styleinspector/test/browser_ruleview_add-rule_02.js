



"use strict";




let PAGE_CONTENT = [
  '<style type="text/css">',
  '  #testid {',
  '    text-align: center;',
  '  }',
  '</style>',
  '<div id="testid">Styled Node</div>',
  '<span>This is a span</span>'
].join("\n");

add_task(function*() {
  yield addTab("data:text/html;charset=utf-8," + encodeURIComponent(PAGE_CONTENT));

  info("Opening the rule-view");
  let {toolbox, inspector, view} = yield openRuleView();

  info("Selecting the test element");
  yield selectNode("#testid", inspector);

  info("Waiting for rule view to update");
  let onRuleViewChanged = once(view, "ruleview-changed");

  info("Adding the new rule");
  view.addRuleButton.click();

  yield onRuleViewChanged;

  yield testEditSelector(view, "span");

  info("Selecting the modified element with the new rule");
  yield selectNode("span", inspector);
  yield checkModifiedElement(view, "span");
});

function* testEditSelector(view, name) {
  info("Test editing existing selector field");
  let idRuleEditor = getRuleViewRuleEditor(view, 1);
  let editor = idRuleEditor.selectorText.ownerDocument.activeElement;

  info("Entering a new selector name and committing");
  editor.value = name;

  info("Waiting for rule view to update");
  let onRuleViewChanged = once(view, "ruleview-changed");

  info("Entering the commit key");
  EventUtils.synthesizeKey("VK_RETURN", {});
  yield onRuleViewChanged;

  is(view._elementStyle.rules.length, 3, "Should have 3 rules.");
  ok(getRuleViewRule(view, name), "Rule with " + name + " selector exists.");
}

function* checkModifiedElement(view, name) {
  is(view._elementStyle.rules.length, 2, "Should have 2 rules.");
  ok(getRuleViewRule(view, name), "Rule with " + name + " selector exists.");
}
