



"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_markup_flashing.html";





const TEST_DATA = [{
  desc: "Adding a new node should flash the new node",
  mutate: (doc, rootNode) => {
    let newLi = doc.createElement("LI");
    newLi.textContent = "new list item";
    rootNode.appendChild(newLi);
  },
  shouldFlash: rootNode => rootNode.lastElementChild
}, {
  desc: "Removing a node should flash its parent",
  mutate: (doc, rootNode) => {
    rootNode.removeChild(rootNode.lastElementChild);
  },
  shouldFlash: rootNode => rootNode
}, {
  desc: "Re-appending an existing node should only flash this node",
  mutate: (doc, rootNode) => {
    rootNode.appendChild(rootNode.firstElementChild);
  },
  shouldFlash: rootNode => rootNode.lastElementChild
}, {
  desc: "Adding an attribute should flash the node",
  mutate: (doc, rootNode) => {
    rootNode.setAttribute("name-" + Date.now(), "value-" + Date.now());
  },
  shouldFlash: rootNode => rootNode
}, {
  desc: "Editing an attribute should flash the node",
  mutate: (doc, rootNode) => {
    rootNode.setAttribute("class", "list value-" + Date.now());
  },
  shouldFlash: rootNode => rootNode
}, {
  desc: "Removing an attribute should flash the node",
  mutate: (doc, rootNode) => {
    rootNode.removeAttribute("class");
  },
  shouldFlash: rootNode => rootNode
}];

let test = asyncTest(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  info("Getting the <ul.list> root node to test mutations on");
  let rootNode = getNode(".list");

  info("Selecting the last element of the root node before starting");
  yield selectNode(rootNode.lastElementChild, inspector);

  for (let {mutate, shouldFlash, desc} of TEST_DATA) {
    info("Starting test: " + desc);

    info("Mutating the DOM and listening for markupmutation event");
    let mutated = inspector.once("markupmutation");
    let updated = inspector.once("inspector-updated");
    mutate(content.document, rootNode);
    yield mutated;

    info("Asserting that the correct markup-container is flashing");
    assertNodeFlashing(shouldFlash(rootNode), inspector);

    
    yield updated;
  }
});

function assertNodeFlashing(node, inspector) {
  let container = getContainerForRawNode(node, inspector);

  if (!container) {
    ok(false, "Node not found");
  } else {
    ok(container.tagState.classList.contains("theme-bg-contrast"),
      "Node is flashing");
  }
}
