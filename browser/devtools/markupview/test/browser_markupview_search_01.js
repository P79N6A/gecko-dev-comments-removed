







const TEST_URL = TEST_URL_ROOT + "doc_markup_search.html";

let test = asyncTest(function*() {
  let {inspector, toolbox} = yield addTab(TEST_URL).then(openInspector);

  ok(!getContainerForRawNode("em", inspector),
    "The <em> tag isn't present yet in the markup-view");

  
  
  
  
  info("searching for the innermost child: <em>");
  let updated = inspector.once("inspector-updated");
  searchUsingSelectorSearch("em", inspector);
  yield updated;

  ok(getContainerForRawNode("em", inspector),
    "The <em> tag is now imported in the markup-view");
  is(inspector.selection.node, getNode("em"),
    "The <em> tag is the currently selected node");

  info("searching for other nodes too");
  for (let node of ["span", "li", "ul"]) {
    let updated = inspector.once("inspector-updated");
    searchUsingSelectorSearch(node, inspector);
    yield updated;
    is(inspector.selection.node, getNode(node),
      "The <" + node + "> tag is the currently selected node");
  }
});
