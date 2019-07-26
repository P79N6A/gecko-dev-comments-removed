


















function runAddAttributesTests(tests, nodeOrSelector, inspector) {
  info("Running " + tests.length + " add-attributes tests");
  return Task.spawn(function*() {
    info("Selecting the test node");
    let div = getNode("div");
    yield selectNode(div, inspector);

    for (let test of tests) {
      yield runAddAttributesTest(test, div, inspector);
    }

    yield inspector.once("inspector-updated");
  });
}






















function* runAddAttributesTest(test, nodeOrSelector, inspector) {
  let element = getNode(nodeOrSelector);

  info("Starting add-attribute test: " + test.desc);
  yield addNewAttributes(element, test.text, inspector);

  info("Assert that the attribute(s) has/have been applied correctly");
  assertAttributes(element, test.expectedAttributes);

  if (test.validate) {
    test.validate(element, getContainerForRawNode(element, inspector), inspector);
  }

  info("Undo the change");
  yield undoChange(inspector);

  info("Assert that the attribute(s) has/have been removed correctly");
  assertAttributes(element, {});
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
  assertAttributes(test.node, test.originalAttributes);

  info("Editing attribute " + test.name + " with value " + test.value);

  let container = getContainerForRawNode(test.node, inspector);
  ok(container && container.editor, "The markup-container for " + test.node +
    " was found");

  info("Listening for the markupmutation event");
  let nodeMutated = inspector.once("markupmutation");
  let attr = container.editor.attrs[test.name].querySelector(".editable");
  setEditableFieldValue(attr, test.value, inspector);
  yield nodeMutated;

  info("Asserting the new attributes after edition");
  assertAttributes(test.node, test.expectedAttributes);

  info("Undo the change and assert that the attributes have been changed back");
  yield undoChange(inspector);
  assertAttributes(test.node, test.originalAttributes);

  info("Redo the change and assert that the attributes have been changed again");
  yield redoChange(inspector);
  assertAttributes(test.node, test.expectedAttributes);
}
