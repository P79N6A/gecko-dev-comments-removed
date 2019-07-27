



"use strict";



const TEST_URL = "data:text/html,<div id='test-div'>Test modifying my ID attribute</div>";

add_task(function*() {
  info("Opening the inspector on the test page");
  let {toolbox, inspector} = yield addTab(TEST_URL).then(openInspector);

  info("Selecting the test node");
  yield selectNode("#test-div", inspector);

  info("Verify attributes, only ID should be there for now");
  yield assertAttributes("#test-div", {
    id: "test-div"
  });

  info("Focus the ID attribute and change its content");
  let {editor} = yield getContainerForSelector("#test-div", inspector);
  let attr = editor.attrElements.get("id").querySelector(".editable");
  let mutated = inspector.once("markupmutation");
  setEditableFieldValue(attr,
    attr.textContent + ' class="newclass" style="color:green"', inspector);
  yield mutated;

  info("Verify attributes, should have ID, class and style");
  yield assertAttributes("#test-div", {
    id: "test-div",
    class: "newclass",
    style: "color:green"
  });

  info("Trying to undo the change");
  yield undoChange(inspector);
  yield assertAttributes("#test-div", {
    id: "test-div"
  });

  yield inspector.once("inspector-updated");
});
