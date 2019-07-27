



"use strict";



const TEST_URI = TEST_URL_ROOT + "doc_keyframeanimation.html";

add_task(function*() {
  yield addTab(TEST_URI);
  let {toolbox, inspector, view} = yield openRuleView();
  yield selectNode("#boxy", inspector);
  yield testAddTextInFilter(inspector, view);
});

function* testAddTextInFilter(inspector, ruleView) {
  info("Setting filter text to \"20%\"");

  let win = ruleView.doc.defaultView;
  let searchField = ruleView.searchField;
  let onRuleViewFilter = inspector.once("ruleview-filtered");

  searchField.focus();
  synthesizeKeys("20%", win);
  yield onRuleViewFilter;

  info("Check that the correct rules are visible");
  is(getRuleViewRuleEditor(ruleView, 0).rule.selectorText, "element", "First rule is inline element.");
  is(getRuleViewRuleEditor(ruleView, 2, 0).rule.domRule.keyText, "20%", "Second rule is 20%.");
  ok(getRuleViewRuleEditor(ruleView, 2, 0).selectorText.classList.contains("ruleview-highlight"),
    "20% selector is highlighted.")
}
