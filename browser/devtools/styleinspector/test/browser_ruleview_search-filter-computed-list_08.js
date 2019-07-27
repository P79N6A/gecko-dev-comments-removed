



"use strict";




const SEARCH = "0px";

let TEST_URI = [
  "<style type='text/css'>",
  "  #testid {",
  "    margin: 4px;",
  "    top: 0px;",
  "  }",
  "</style>",
  "<h1 id='testid'>Styled Node</h1>"
].join("\n");

add_task(function*() {
  yield addTab("data:text/html;charset=utf-8," + encodeURIComponent(TEST_URI));
  let {inspector, view} = yield openRuleView();
  yield selectNode("#testid", inspector);
  yield testModifyPropertyValueFilter(inspector, view);
});

function* testModifyPropertyValueFilter(inspector, view) {
  info("Setting filter text to \"" + SEARCH + "\"");

  let searchField = view.searchField;
  let onRuleViewFiltered = inspector.once("ruleview-filtered");

  searchField.focus();
  synthesizeKeys(SEARCH, view.styleWindow);
  yield onRuleViewFiltered;

  let rule = getRuleViewRuleEditor(view, 1).rule;
  let propEditor = rule.textProps[0].editor;
  let computed = propEditor.computed;
  let editor = yield focusEditableField(view, propEditor.valueSpan);

  info("Check that the correct rules are visible");
  is(rule.selectorText, "#testid", "Second rule is #testid.");
  ok(!propEditor.container.classList.contains("ruleview-highlight"),
    "margin text property is not highlighted.");
  ok(rule.textProps[1].editor.container.classList.contains("ruleview-highlight"),
    "top text property is correctly highlighted.");

  let onBlur = once(editor.input, "blur");
  let onModification = rule._applyingModifications;
  EventUtils.sendString("4px 0px", view.styleWindow);
  EventUtils.synthesizeKey("VK_RETURN", {});
  yield onBlur;
  yield onModification;

  ok(propEditor.container.classList.contains("ruleview-highlight"),
    "margin text property is correctly highlighted.");
  ok(!computed.hasAttribute("filter-open"), "margin computed list is closed.");
  ok(!computed.children[0].classList.contains("ruleview-highlight"),
    "margin-top computed property is not highlighted.");
  ok(computed.children[1].classList.contains("ruleview-highlight"),
    "margin-right computed property is correctly highlighted.");
  ok(!computed.children[2].classList.contains("ruleview-highlight"),
    "margin-bottom computed property is not highlighted.");
  ok(computed.children[3].classList.contains("ruleview-highlight"),
    "margin-left computed property is correctly highlighted.");
}
