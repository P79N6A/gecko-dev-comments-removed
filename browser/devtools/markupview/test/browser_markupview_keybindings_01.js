



"use strict";



const TEST_URL = "data:text/html;charset=utf8,<div a b c d e id='test'></div>";

add_task(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  info("Focusing the tag editor of the test element");
  let {editor} = yield getContainerForSelector("div", inspector);
  editor.tag.focus();

  info("Pressing tab and expecting to focus the ID attribute, always first");
  EventUtils.sendKey("tab", inspector.panelWin);
  checkFocusedAttribute("id");

  info("Hit enter to turn the attribute to edit mode");
  EventUtils.sendKey("return", inspector.panelWin);
  checkFocusedAttribute("id", true);

  
  
  let attributes = [...getNode("div").attributes].filter(attr => attr.name !== "id");

  info("Tabbing forward through attributes in edit mode");
  for (let {name} of attributes) {
    collapseSelectionAndTab(inspector);
    checkFocusedAttribute(name, true);
  }

  info("Tabbing backward through attributes in edit mode");

  
  
  let reverseAttributes = attributes.reverse();
  reverseAttributes.shift();

  for (let {name} of reverseAttributes) {
    collapseSelectionAndShiftTab(inspector);
    checkFocusedAttribute(name, true);
  }
});
