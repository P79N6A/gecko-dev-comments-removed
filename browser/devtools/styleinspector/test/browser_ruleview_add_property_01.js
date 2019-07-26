



"use strict";




const TEST_URI = TEST_URL_ROOT + "browser_bug705707_is_content_stylesheet.html";

let test = asyncTest(function*() {
  yield addTab(TEST_URI);

  let target = getNode("#target");

  let {toolbox, inspector, view} = yield openRuleView();
  yield selectNode(target, inspector);

  info("Setting a font-weight property on all rules");
  setPropertyOnAllRules(view);

  info("Reselecting the element");
  yield reselectElement(target, inspector);

  checkPropertyOnAllRules(view);
});

function reselectElement(node, inspector) {
  return selectNode(node.parentNode, inspector).then(() => {
    return selectNode(node, inspector);
  });
}

function setPropertyOnAllRules(view) {
  for (let rule of view._elementStyle.rules) {
    rule.editor.addProperty("font-weight", "bold", "");
  }
}

function checkPropertyOnAllRules(view) {
  for (let rule of view._elementStyle.rules) {
    let lastRule = rule.textProps[rule.textProps.length - 1];

    is(lastRule.name, "font-weight", "Last rule name is font-weight");
    is(lastRule.value, "bold", "Last rule value is bold");
  }
}
