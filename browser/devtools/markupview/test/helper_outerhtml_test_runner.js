
















function runEditOuterHTMLTests(tests, inspector) {
  info("Running " + tests.length + " edit-outer-html tests");
  return Task.spawn(function* () {
    for (let step of TEST_DATA) {
      yield runEditOuterHTMLTest(step, inspector);
    }
  });
}














function* runEditOuterHTMLTest(test, inspector) {
  info("Running an edit outerHTML test on '" + test.selector + "'");
  yield selectNode(test.selector, inspector);
  let oldNodeFront = inspector.selection.nodeFront;

  info("Listening for the markupmutation event");
  
  let mutated = inspector.once("markupmutation");
  info("Editing the outerHTML");
  inspector.markup.updateNodeOuterHTML(inspector.selection.nodeFront, test.newHTML, test.oldHTML);
  let mutations = yield mutated;
  ok(true, "The markupmutation event has fired, mutation done");

  info("Check to make the sure the correct mutation event was fired, and that the parent is selected");
  let nodeFront = inspector.selection.nodeFront;
  let mutation = mutations[0];
  let isFromOuterHTML = mutation.removed.some(n => n === oldNodeFront);

  ok(isFromOuterHTML, "The node is in the 'removed' list of the mutation");
  is(mutation.type, "childList", "Mutation is a childList after updating outerHTML");
  is(mutation.target, nodeFront, "Parent node is selected immediately after setting outerHTML");

  
  yield inspector.selection.once("new-node");

  
  
  let selectedNode = inspector.selection.node;
  let nodeFront = inspector.selection.nodeFront;
  let pageNode = getNode(test.selector);

  if (test.validate) {
    test.validate(pageNode, selectedNode);
  } else {
    is(pageNode, selectedNode, "Original node (grabbed by selector) is selected");
    is(pageNode.outerHTML, test.newHTML, "Outer HTML has been updated");
  }

  
  
  yield inspector.once("inspector-updated");
}
