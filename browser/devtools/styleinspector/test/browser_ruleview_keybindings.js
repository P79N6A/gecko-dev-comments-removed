



"use strict";




add_task(function*() {
  yield addTab("data:text/html;charset=utf-8,<h1>Some header text</h1>");
  let {toolbox, inspector, view} = yield openRuleView();

  info("Selecting the test node");
  yield selectNode("h1", inspector);

  info("Getting the ruleclose brace element");
  let brace = view.styleDocument.querySelector(".ruleview-ruleclose");

  info("Clicking on the brace element to focus the new property field");
  let onFocus = once(brace.parentNode, "focus", true);
  brace.click();
  yield onFocus;

  info("Entering a property name");
  let editor = getCurrentInplaceEditor(view);
  editor.input.value = "color";

  info("Typing ENTER to focus the next field: property value");
  onFocus = once(brace.parentNode, "focus", true);
  
  
  let onRuleViewChanged = view.once("ruleview-changed").then(() => view.once("ruleview-changed"));
  EventUtils.sendKey("return");
  yield onFocus;
  yield onRuleViewChanged;
  ok(true, "The value field was focused");

  info("Entering a property value");
  editor = getCurrentInplaceEditor(view);
  editor.input.value = "green";

  info("Typing ENTER again should focus a new property name");
  onFocus = once(brace.parentNode, "focus", true);
  onRuleViewChanged = view.once("ruleview-changed");
  EventUtils.sendKey("return");
  yield onFocus;
  yield onRuleViewChanged;
  ok(true, "The new property name field was focused");
  getCurrentInplaceEditor(view).input.blur();
});

function getCurrentInplaceEditor(view) {
  return inplaceEditor(view.styleDocument.activeElement);
}
