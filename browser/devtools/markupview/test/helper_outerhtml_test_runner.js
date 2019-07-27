
















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

  let onUpdated = inspector.once("inspector-updated");

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

  
  yield inspector.selection.once("new-node-front");

  
  
  let selectedNodeFront = inspector.selection.nodeFront;
  let pageNodeFront = yield inspector.walker.querySelector(inspector.walker.rootNode, test.selector);
  let pageNode = getNode(test.selector);

  if (test.validate) {
    yield test.validate(pageNode, pageNodeFront, selectedNodeFront, inspector);
  } else {
    is(pageNodeFront, selectedNodeFront, "Original node (grabbed by selector) is selected");
    is(pageNode.outerHTML, test.newHTML, "Outer HTML has been updated");
  }

  
  
  yield onUpdated;
}
