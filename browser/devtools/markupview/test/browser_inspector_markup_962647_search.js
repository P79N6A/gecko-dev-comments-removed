







const TEST_URL = "http://mochi.test:8888/browser/browser/devtools/markupview/test/browser_inspector_markup_962647_search.html";

function test() {
  waitForExplicitFinish();

  let p = content.document.querySelector("p");
  Task.spawn(function() {
    info("loading the test page");
    yield addTab(TEST_URL);

    info("opening the inspector");
    let {inspector, toolbox} = yield openInspector();

    ok(!getContainerForRawNode(inspector.markup, getNode("em")),
      "The <em> tag isn't present yet in the markup-view");

    
    
    
    
    info("searching for the innermost child: <em>");
    let updated = inspector.once("inspector-updated");
    searchUsingSelectorSearch("em", inspector);
    yield updated;

    ok(getContainerForRawNode(inspector.markup, getNode("em")),
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

    gBrowser.removeCurrentTab();
  }).then(null, ok.bind(null, false)).then(finish);
}
