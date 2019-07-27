







let PAGE_1 = TEST_URL_ROOT + "browser_inspector_select_last_selected.html";
let PAGE_2 = TEST_URL_ROOT + "browser_inspector_select_last_selected2.html";






let TEST_DATA = [
  {
    url: PAGE_1,
    nodeToSelect: "#id1",
    selectedNode: "#id1"
  },
  {
    url: PAGE_1,
    nodeToSelect: "#id2",
    selectedNode: "#id2"
  },
  {
    url: PAGE_1,
    nodeToSelect: "#id3",
    selectedNode: "#id3"
  },
  {
    url: PAGE_1,
    nodeToSelect: "#id4",
    selectedNode: "#id4"
  },
  {
    url: PAGE_2,
    nodeToSelect: null,
    selectedNode: "body"
  },
  {
    url: PAGE_1,
    nodeToSelect: "#id5",
    selectedNode: "body"
  },
  {
    url: PAGE_2,
    nodeToSelect: null,
    selectedNode: "body"
  }
];

let test = asyncTest(function* () {
  let { inspector } = yield openInspectorForURL(PAGE_1);

  for (let { url, nodeToSelect, selectedNode } of TEST_DATA) {
    if (nodeToSelect) {
      info("Selecting node " + nodeToSelect + " before navigation.");
      yield selectNode(nodeToSelect, inspector);
    }

    yield navigateToAndWaitForNewRoot(url);

    info("Waiting for inspector to update after new-root event.");
    yield inspector.once("inspector-updated");

    is(inspector.selection.node, getNode(selectedNode),
       selectedNode + " is selected after navigation.");
  }

  function navigateToAndWaitForNewRoot(url) {
    info("Navigating and waiting for new-root event after navigation.");
    let newRoot = inspector.once("new-root");

    if (url == content.location) {
      info("Reloading page.");
      content.location.reload();
    } else {
      info("Navigating to " + url);
      content.location = url;
    }

    return newRoot;
  }
});
