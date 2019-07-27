



"use strict";



let TEST_URI = [
  '<style type="text/css">',
  '  #testid {',
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
  yield testClearSearchFilter(inspector, view);
});

function* testAddTextInFilter(inspector, ruleView) {
  info("Setting filter text to \"00F\"");

  let win = ruleView.doc.defaultView;
  let searchField = ruleView.searchField;
  let onRuleViewFiltered = inspector.once("ruleview-filtered");

  searchField.focus();
  synthesizeKeys("00F", win);
  yield onRuleViewFiltered;

  info("Check that the correct rules are visible");
  is(ruleView.element.children.length, 2, "Should have 2 rules.");
  is(getRuleViewRuleEditor(ruleView, 0).rule.selectorText, "element", "First rule is inline element.");
  is(getRuleViewRuleEditor(ruleView, 1).rule.selectorText, "#testid", "Second rule is #testid.");
  ok(getRuleViewRuleEditor(ruleView, 1).rule.textProps[0].editor.element.classList.contains("ruleview-highlight"),
    "background-color text property is correctly highlighted.");
}

function* testClearSearchFilter(inspector, ruleView) {
  info("Clearing the search filter");

  let doc = ruleView.doc;
  let win = ruleView.doc.defaultView;
  let searchField = ruleView.searchField;
  let searchClearButton = ruleView.searchClearButton;
  let onRuleViewFiltered = inspector.once("ruleview-filtered");

  EventUtils.synthesizeMouseAtCenter(searchClearButton, {}, win);

  yield onRuleViewFiltered;

  info("Check the search filter is cleared and no rules are highlighted");
  is(ruleView.element.children.length, 3, "Should have 3 rules.");
  ok(!searchField.value, "Search filter is cleared");
  ok(!doc.querySelectorAll(".ruleview-highlight").length &&
     !ruleView._highlightedElements.length, "No rules are higlighted");
}
