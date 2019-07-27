


















function runAddAttributesTests(tests, nodeOrSelector, inspector) {
  info("Running " + tests.length + " add-attributes tests");
  return Task.spawn(function*() {
    info("Selecting the test node");
    yield selectNode("div", inspector);

    for (let test of tests) {
      yield runAddAttributesTest(test, "div", inspector);
    }
  });
}





















function* runAddAttributesTest(test, selector, inspector) {
  let element = getNode(selector);

  info("Starting add-attribute test: " + test.desc);
  yield addNewAttributes(selector, test.text, inspector);

  info("Assert that the attribute(s) has/have been applied correctly");
  yield assertAttributes(selector, test.expectedAttributes);

  if (test.validate) {
    let container = yield getContainerForSelector(selector, inspector);
    test.validate(element, container, inspector);
  }

  info("Undo the change");
  yield undoChange(inspector);

  info("Assert that the attribute(s) has/have been removed correctly");
  yield assertAttributes(selector, {});
}














function runEditAttributesTests(tests, inspector) {
  info("Running " + tests.length + " edit-attributes tests");
  return Task.spawn(function*() {
    info("Expanding all nodes in the markup-view");
    yield inspector.markup.expandAll();

    for (let test of tests) {
      yield runEditAttributesTest(test, inspector);
    }

    yield inspector.once("inspector-updated");
  });
}


















function* runEditAttributesTest(test, inspector) {
  info("Starting edit-attribute test: " + test.desc);

  info("Selecting the test node " + test.node);
  yield selectNode(test.node, inspector);

  info("Asserting that the node has the right attributes to start with");
  yield assertAttributes(test.node, test.originalAttributes);

  info("Editing attribute " + test.name + " with value " + test.value);

  let container = yield getContainerForSelector(test.node, inspector);
  ok(container && container.editor, "The markup-container for " + test.node +
    " was found");

  info("Listening for the markupmutation event");
  let nodeMutated = inspector.once("markupmutation");
  let attr = container.editor.attrElements.get(test.name).querySelector(".editable");
  setEditableFieldValue(attr, test.value, inspector);
  yield nodeMutated;

  info("Asserting the new attributes after edition");
  yield assertAttributes(test.node, test.expectedAttributes);

  info("Undo the change and assert that the attributes have been changed back");
  yield undoChange(inspector);
  yield assertAttributes(test.node, test.originalAttributes);

  info("Redo the change and assert that the attributes have been changed again");
  yield redoChange(inspector);
  yield assertAttributes(test.node, test.expectedAttributes);
}
