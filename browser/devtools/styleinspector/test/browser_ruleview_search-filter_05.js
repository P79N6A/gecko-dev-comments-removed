



"use strict";



let TEST_URI = [
  '<style type="text/css">',
  '  html, body, div {',
  '    background-color: #00F;',
  '  }',
  '  .testclass {',
  '    width: 100%;',
  '  }',
  '</style>',
  '<div id="testid" class="testclass">Styled Node</div>'
].join("\n");

add_task(function*() {
  yield addTab("data:text/html;charset=utf-8," + encodeURIComponent(TEST_URI));
  let {toolbox, inspector, view} = yield openRuleView();
  yield selectNode("#testid", inspector);
  yield testAddTextInFilter(inspector, view);
});

function* testAddTextInFilter(inspector, ruleView) {
  info("Setting filter text to \"body\"");

  let win = ruleView.doc.defaultView;
  let searchField = ruleView.searchField;
  let onRuleViewFilter = inspector.once("ruleview-filtered");

  searchField.focus();
  synthesizeKeys("body", win);
  yield onRuleViewFilter;

  info("Check that the correct rules are visible");
  is(ruleView.element.children.length, 2, "Should have 2 rules.");
  is(getRuleViewRuleEditor(ruleView, 0).rule.selectorText, "element", "First rule is inline element.");
  is(getRuleViewRuleEditor(ruleView, 1).rule.selectorText, "html, body, div", "Second rule is html, body, div.");
  ok(getRuleViewRuleEditor(ruleView, 1).selectorText.children[2].classList.contains("ruleview-highlight"),
    "body selector is highlighted.")
}
