




"use strict";





const TEST_PAGE = TEST_URL_ROOT +
  "doc_inspector_delete-selected-node-02.html";

let test = asyncTest(function* () {
  let { inspector } = yield openInspectorForURL(TEST_PAGE);

  yield testManuallyDeleteSelectedNode();
  yield testAutomaticallyDeleteSelectedNode();
  yield testDeleteSelectedNodeContainerFrame();

  function* testManuallyDeleteSelectedNode() {
    info("Selecting a node, deleting it via context menu and checking that " +
          "its parent node is selected and breadcrumbs are updated.");

    let div = getNode("#deleteManually");
    yield selectNode(div, inspector);

    info("Getting the node container in the markup view.");
    let container = getContainerForRawNode(inspector.markup, div);

    info("Simulating right-click on the markup view container.");
    EventUtils.synthesizeMouse(container.tagLine, 2, 2,
      {type: "contextmenu", button: 2}, inspector.panelWin);

    info("Waiting for the context menu to open.");
    yield once(inspector.panelDoc.getElementById("inspectorPopupSet"), "popupshown");

    info("Clicking 'Delete Node' in the context menu.");
    inspector.panelDoc.getElementById("node-menu-delete").click();

    info("Waiting for inspector to update.");
    yield inspector.once("inspector-updated");

    info("Inspector updated, performing checks.");
    let parent = getNode("#deleteChildren");
    assertNodeSelectedAndPanelsUpdated(parent, "ul#deleteChildren");
  }

  function* testAutomaticallyDeleteSelectedNode() {
    info("Selecting a node, deleting it via javascript and checking that " +
         "its parent node is selected and breadcrumbs are updated.");

    let div = getNode("#deleteAutomatically");
    yield selectNode(div, inspector);

    info("Deleting selected node via javascript.");
    div.remove();

    info("Waiting for inspector to update.");
    yield inspector.once("inspector-updated");

    info("Inspector updated, performing checks.");
    let parent = getNode("#deleteChildren");
    assertNodeSelectedAndPanelsUpdated(parent, "ul#deleteChildren");
  }

  function* testDeleteSelectedNodeContainerFrame() {
    info("Selecting a node inside iframe, deleting the iframe via javascript " +
         "and checking the parent node of the iframe is selected and " +
         "breadcrumbs are updated.");

    info("Selecting an element inside iframe.");
    let iframe = getNode("#deleteIframe");
    let div = iframe.contentDocument.getElementById("deleteInIframe");
    yield selectNode(div, inspector);

    info("Deleting selected node via javascript.");
    iframe.remove();

    info("Waiting for inspector to update.");
    yield inspector.once("inspector-updated");

    info("Inspector updated, performing checks.");
    assertNodeSelectedAndPanelsUpdated(getNode("body"), "body");
  }

  function assertNodeSelectedAndPanelsUpdated(node, crumbLabel) {
    is(inspector.selection.nodeFront, getNodeFront(node),
      "The right node is selected");

    let breadcrumbs = inspector.panelDoc.getElementById("inspector-breadcrumbs");
    is(breadcrumbs.querySelector("button[checked=true]").textContent, crumbLabel,
      "The right breadcrumb is selected");
  }
});
