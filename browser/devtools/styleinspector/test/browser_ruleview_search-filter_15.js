



"use strict";




const SEARCH = "e";

let TEST_URI = [
  "<style type='text/css'>",
  "  #testid {",
  "    width: 100%;",
  "    height: 50%;",
  "  }",
  "</style>",
  "<h1 id='testid'>Styled Node</h1>"
].join("\n");

add_task(function*() {
  yield addTab("data:text/html;charset=utf-8," + encodeURIComponent(TEST_URI));
  let {inspector, view} = yield openRuleView();
  yield selectNode("#testid", inspector);
  yield testModifyPropertyNameFilter(inspector, view);
});

function* testModifyPropertyNameFilter(inspector, view) {
  info("Setting filter text to \"" + SEARCH + "\"");

  let searchField = view.searchField;
  let onRuleViewFiltered = inspector.once("ruleview-filtered");

  searchField.focus();
  synthesizeKeys(SEARCH, view.styleWindow);
  yield onRuleViewFiltered;

  let ruleEditor = getRuleViewRuleEditor(view, 1);
  let rule = ruleEditor.rule;
  let propEditor = rule.textProps[0].editor;
  let editor = yield focusEditableField(view, propEditor.nameSpan);

  info("Check that the correct rules are visible");
  is(view.element.children.length, 2, "Should have 2 rules.");
  is(rule.selectorText, "#testid", "Second rule is #testid.");
  ok(!propEditor.container.classList.contains("ruleview-highlight"),
    "width text property is not highlighted.");
  ok(rule.textProps[1].editor.container.classList.contains("ruleview-highlight"),
    "height text property is correctly highlighted.");

  let onBlur = once(editor.input, "blur");
  let onModification = rule._applyingModifications;
  EventUtils.sendString("margin-left", view.styleWindow);
  EventUtils.synthesizeKey("VK_RETURN", {});
  yield onBlur;
  yield onModification;

  ok(propEditor.container.classList.contains("ruleview-highlight"),
    "margin-left text property is correctly highlighted.");
}

