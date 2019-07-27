







let PAGE_1 = TEST_URL_ROOT + "doc_inspector_select-last-selected-01.html";
let PAGE_2 = TEST_URL_ROOT + "doc_inspector_select-last-selected-02.html";






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

add_task(function* () {
  let { inspector, toolbox, testActor } = yield openInspectorForURL(PAGE_1);

  for (let { url, nodeToSelect, selectedNode } of TEST_DATA) {
    if (nodeToSelect) {
      info("Selecting node " + nodeToSelect + " before navigation.");
      yield selectNode(nodeToSelect, inspector);
    }

    let onNewRoot = inspector.once("new-root");
    yield navigateToAndWaitForNewRoot(toolbox, testActor, url);

    info("Waiting for new root.");
    yield onNewRoot;

    info("Waiting for inspector to update after new-root event.");
    yield inspector.once("inspector-updated");

    let nodeFront = yield getNodeFront(selectedNode, inspector);
    ok(nodeFront, "Got expected node front");
    is(inspector.selection.nodeFront, nodeFront,
       selectedNode + " is selected after navigation.");
  }

  function navigateToAndWaitForNewRoot(toolbox, testActor, url) {
    info("Navigating and waiting for new-root event after navigation.");

    let newRoot = inspector.once("new-root");

    return testActor.eval("location.href")
      .then(current => {
        if (url == current) {
          info("Reloading page.");
          let activeTab = toolbox.target.activeTab;
          return activeTab.reload();
        } else {
          info("Navigating to " + url);
          navigateTo(toolbox, url);
        }

        return newRoot;
      });
  }
});
