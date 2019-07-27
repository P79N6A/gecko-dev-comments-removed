



"use strict";




const SEARCH = "0px"

let TEST_URI = [
  '<style type="text/css">',
  '  #testid {',
  '    margin: 4px 0px;',
  '  }',
  '  .testclass {',
  '    background-color: red;',
  '  }',
  '</style>',
  '<h1 id="testid" class="testclass">Styled Node</h1>'
].join("\n");

add_task(function*() {
  yield addTab("data:text/html;charset=utf-8," + encodeURIComponent(TEST_URI));
  let {toolbox, inspector, view} = yield openRuleView();
  yield selectNode("#testid", inspector);
  yield testOpenExpanderAndAddTextInFilter(inspector, view);
  yield testClearSearchFilter(inspector, view);
});

function* testOpenExpanderAndAddTextInFilter(inspector, ruleView) {
  let win = ruleView.doc.defaultView;
  let searchField = ruleView.searchField;
  let onRuleViewFiltered = inspector.once("ruleview-filtered");
  let rule = getRuleViewRuleEditor(ruleView, 1).rule;
  let ruleEditor = rule.textProps[0].editor;
  let computed = ruleEditor.computed;

  info("Opening the computed list of margin property")
  ruleEditor.expander.click();

  info("Setting filter text to \"" + SEARCH + "\"");
  searchField.focus();
  synthesizeKeys(SEARCH, win);
  yield onRuleViewFiltered;

  info("Check that the correct rules are visible");
  is(ruleView.element.children.length, 2, "Should have 2 rules.");
  is(getRuleViewRuleEditor(ruleView, 0).rule.selectorText, "element",
    "First rule is inline element.");

  is(rule.selectorText, "#testid", "Second rule is #testid.");
  ok(ruleEditor.expander.getAttribute("open"), "Expander is open.");
  ok(ruleEditor.container.classList.contains("ruleview-highlight"),
    "margin text property is correctly highlighted.");
  ok(!computed.classList.contains("filter-open"),
    "margin computed list does not contain filter-open class.");
  ok(computed.classList.contains("styleinspector-open"),
    "margin computed list contains styleinspector-open class.");

  ok(!computed.children[0].classList.contains("ruleview-highlight"),
    "margin-top computed property is not highlighted.");
  ok(computed.children[1].classList.contains("ruleview-highlight"),
    "margin-right computed property is correctly highlighted.");
  ok(!computed.children[2].classList.contains("ruleview-highlight"),
    "margin-bottom computed property is not highlighted.");
  ok(computed.children[3].classList.contains("ruleview-highlight"),
    "margin-left computed property is correctly highlighted.");
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
  ok(!doc.querySelectorAll(".ruleview-highlight").length,
    "No rules are higlighted");

  let ruleEditor = getRuleViewRuleEditor(ruleView, 1).rule.textProps[0].editor;
  let computed = ruleEditor.computed;

  ok(ruleEditor.expander.getAttribute("open"), "Expander is open.");
  ok(!computed.classList.contains("filter-open"),
    "margin computed list does not contain filter-open class.");
  ok(computed.classList.contains("styleinspector-open"),
    "margin computed list contains styleinspector-open class.");
}
