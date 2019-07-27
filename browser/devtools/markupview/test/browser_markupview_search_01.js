



"use strict";





const TEST_URL = TEST_URL_ROOT + "doc_markup_search.html";

add_task(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  let container = yield getContainerForSelector("em", inspector);
  ok(!container, "The <em> tag isn't present yet in the markup-view");

  
  
  
  
  
  info("searching for the innermost child: <em>");
  yield searchFor("em", inspector);

  container = yield getContainerForSelector("em", inspector);
  ok(container, "The <em> tag is now imported in the markup-view");

  let nodeFront = yield getNodeFront("em", inspector);
  is(inspector.selection.nodeFront, nodeFront,
    "The <em> tag is the currently selected node");

  info("searching for other nodes too");
  for (let node of ["span", "li", "ul"]) {
    yield searchFor(node, inspector);

    nodeFront = yield getNodeFront(node, inspector);
    is(inspector.selection.nodeFront, nodeFront,
      "The <" + node + "> tag is the currently selected node");
  }
});

function* searchFor(selector, inspector) {
  let onNewNodeFront = inspector.selection.once("new-node-front");

  searchUsingSelectorSearch(selector, inspector);

  yield onNewNodeFront;
  yield inspector.once("inspector-updated");
}
