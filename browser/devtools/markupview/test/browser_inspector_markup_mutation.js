












const TEST_URL = TEST_URL_ROOT + "browser_inspector_markup_mutation.html";

const TEST_DATA = [
  {
    desc: "Adding an attribute",
    test: () => {
      let node1 = getNode("#node1");
      node1.setAttribute("newattr", "newattrval");
    }
  },
  {
    desc: "Removing an attribute",
    test: () => {
      let node1 = getNode("#node1");
      node1.removeAttribute("newattr");
    }
  },
  {
    desc: "Updating the text-content",
    test: () => {
      let node1 = getNode("#node1");
      node1.textContent = "newtext";
    }
  },
  {
    desc: "Updating the innerHTML",
    test: () => {
      let node2 = getNode("#node2");
      node2.innerHTML = "<div><span>foo</span></div>";
    }
  },
  {
    desc: "Removing child nodes",
    test: () => {
      let node4 = getNode("#node4");
      while (node4.firstChild) {
        node4.removeChild(node4.firstChild);
      }
    }
  },
  {
    desc: "Appending a child to a different parent",
    test: () => {
      let node17 = getNode("#node17");
      let node1 = getNode("#node2");
      node1.appendChild(node17);
    }
  },
  {
    desc: "Swapping a parent and child element, putting them in the same tree",
    
    
    
    
    
    
    
    
    
    
    
    
    
    test: () => {
      let node18 = getNode("#node18");
      let node20 = getNode("#node20");

      let node1 = getNode("#node1");

      node1.appendChild(node20);
      node20.appendChild(node18);
    }
  }
];

let test = asyncTest(function*() {
  info("Creating the helper tab for parsing");
  let parseTab = yield addTab("data:text/html,<html></html>");
  let parseDoc = content.document;

  info("Creating the test tab");
  let contentTab = yield addTab(TEST_URL);
  let doc = content.document;
  
  stripWhitespace(doc.documentElement);

  let {inspector} = yield openInspector();
  let markup = inspector.markup;

  info("Expanding all markup-view nodes");
  yield markup.expandAll();

  for (let step of TEST_DATA) {
    info("Starting test: " + step.desc);

    info("Executing the test markup mutation, listening for inspector-updated before moving on");
    let updated = inspector.once("inspector-updated");
    step.test();
    yield updated;

    info("Expanding all markup-view nodes to make sure new nodes are imported");
    yield markup.expandAll();

    info("Comparing the markup-view markup with the content document");
    compareMarkup(parseDoc, inspector);
  }
});

function stripWhitespace(node) {
  node.normalize();
  let iter = node.ownerDocument.createNodeIterator(node,
    NodeFilter.SHOW_TEXT + NodeFilter.SHOW_COMMENT, null);

  while ((node = iter.nextNode())) {
    node.nodeValue = node.nodeValue.replace(/\s+/g, '');
    if (node.nodeType == Node.TEXT_NODE &&
      !/[^\s]/.exec(node.nodeValue)) {
      node.parentNode.removeChild(node);
    }
  }
}

function compareMarkup(parseDoc, inspector) {
  
  let markupContainerEl = getContainerForRawNode("body", inspector).elt;
  let sel = markupContainerEl.ownerDocument.defaultView.getSelection();
  sel.selectAllChildren(markupContainerEl);

  
  let parseNode = parseDoc.querySelector("body");
  parseNode.outerHTML = sel;
  parseNode = parseDoc.querySelector("body");

  
  
  stripWhitespace(parseNode);

  
  ok(getNode("body").isEqualNode(parseNode),
    "Markup panel matches what's in the content document.");
}
