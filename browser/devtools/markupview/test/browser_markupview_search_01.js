



"use strict";





const TEST_URL = TEST_URL_ROOT + "doc_markup_search.html";

let test = asyncTest(function*() {
  let {inspector, toolbox} = yield addTab(TEST_URL).then(openInspector);

  let container = yield getContainerForSelector("em", inspector);
  ok(!container, "The <em> tag isn't present yet in the markup-view");

  
  
  
  
  info("searching for the innermost child: <em>");
  let updated = inspector.once("inspector-updated");
  searchUsingSelectorSearch("em", inspector);
  yield updated;

  container = yield getContainerForSelector("em", inspector);
  ok(container, "The <em> tag is now imported in the markup-view");

  let nodeFront = yield getNodeFront("em", inspector);
  is(inspector.selection.nodeFront, nodeFront,
    "The <em> tag is the currently selected node");

  info("searching for other nodes too");
  for (let node of ["span", "li", "ul"]) {
    let updated = inspector.once("inspector-updated");
    searchUsingSelectorSearch(node, inspector);
    yield updated;

    nodeFront = yield getNodeFront(node, inspector);
    is(inspector.selection.nodeFront, nodeFront,
      "The <" + node + "> tag is the currently selected node");
  }
});
