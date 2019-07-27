
















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

  info("Listen for reselectedonremoved and edit the outerHTML");
  let onReselected = inspector.markup.once("reselectedonremoved");
  yield inspector.markup.updateNodeOuterHTML(inspector.selection.nodeFront,
                                             test.newHTML, test.oldHTML);
  yield onReselected;

  
  
  let selectedNodeFront = inspector.selection.nodeFront;
  let pageNodeFront = yield inspector.walker.querySelector(inspector.walker.rootNode, test.selector);
  let pageNode = getNode(test.selector);

  if (test.validate) {
    yield test.validate(pageNode, pageNodeFront, selectedNodeFront, inspector);
  } else {
    is(pageNodeFront, selectedNodeFront, "Original node (grabbed by selector) is selected");
    let {outerHTML} = yield getNodeInfo(test.selector);
    is(outerHTML, test.newHTML, "Outer HTML has been updated");
  }

  
  
  yield onUpdated;
}
