



"use strict";




const TEST_URL = "data:text/html;charset=utf8,<div id='attr' c='3' b='2' a='1'></div><div id='delattr' last='1' tobeinvalid='2'></div>";

add_task(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  yield testAttributeEditing(inspector);
  yield testAttributeDeletion(inspector);
});

function* testAttributeEditing(inspector) {
  info("Testing focus position after attribute editing");

  
  
  
  
  
  yield selectNode("#attr", inspector);

  info("Setting the first non-id attribute in edit mode");
  yield activateFirstAttribute("#attr", inspector); 
  collapseSelectionAndTab(inspector); 

  
  
  
  
  let attrs = getNodeAttributesOtherThanId("#attr");

  info("Editing this attribute, keeping the same name, and tabbing to the next");
  yield editAttributeAndTab(attrs[0].name + '="99"', inspector);
  checkFocusedAttribute(attrs[1].name, true);

  info("Editing the new focused attribute, keeping the name, and tabbing to the previous");
  yield editAttributeAndTab(attrs[1].name + '="99"', inspector, true);
  checkFocusedAttribute(attrs[0].name, true);

  info("Editing attribute name, changes attribute order");
  yield editAttributeAndTab("d='4'", inspector);
  checkFocusedAttribute("id", true);

  
  EventUtils.sendKey("escape", inspector.panelWin);
}

function* testAttributeDeletion(inspector) {
  info("Testing focus position after attribute deletion");

  
  
  
  
  
  yield selectNode("#delattr", inspector);

  info("Setting the first non-id attribute in edit mode");
  yield activateFirstAttribute("#delattr", inspector); 
  collapseSelectionAndTab(inspector); 

  
  
  
  
  let attrs = getNodeAttributesOtherThanId("#delattr");

  info("Entering an invalid attribute to delete the attribute");
  yield editAttributeAndTab('"', inspector);
  checkFocusedAttribute(attrs[1].name, true);

  info("Deleting the last attribute");
  yield editAttributeAndTab(" ", inspector);

  
  let focusedAttr = Services.focus.focusedElement;
  ok(focusedAttr.classList.contains("styleinspector-propertyeditor"), "in newattr");
  is(focusedAttr.tagName, "input", "newattr is active");
}

function* editAttributeAndTab(newValue, inspector, goPrevious) {
  var onEditMutation = inspector.markup.once("refocusedonedit");
  inspector.markup.doc.activeElement.value = newValue;
  if (goPrevious) {
    EventUtils.synthesizeKey("VK_TAB", { shiftKey: true },
      inspector.panelWin);
  } else {
    EventUtils.sendKey("tab", inspector.panelWin);
  }
  yield onEditMutation;
}





function* activateFirstAttribute(container, inspector) {
  let {editor} = yield getContainerForSelector(container, inspector);
  editor.tag.focus();

  
  EventUtils.sendKey("tab", inspector.panelWin);
  EventUtils.sendKey("return", inspector.panelWin);
}

function getNodeAttributesOtherThanId(selector) {
  return [...getNode(selector).attributes].filter(attr => attr.name !== "id");
}
