



"use strict";



const TEST_URI = "http://example.com/browser/browser/devtools/inspector/" +
                 "test/doc_inspector_infobar_01.html";

add_task(function*() {
  yield addTab(TEST_URI);
  let {inspector} = yield openInspector();

  let testData = [
    {
      selector: "#top",
      position: "bottom",
      tag: "DIV",
      id: "top",
      classes: ".class1.class2",
      dims: "500 x 100"
    },
    {
      selector: "#vertical",
      position: "overlap",
      tag: "DIV",
      id: "vertical",
      classes: ""
      
    },
    {
      selector: "#bottom",
      position: "top",
      tag: "DIV",
      id: "bottom",
      classes: "",
      dims: "500 x 100"
    },
    {
      selector: "body",
      position: "bottom",
      tag: "BODY",
      classes: ""
      
    },
  ];

  for (let currTest of testData) {
    yield testPosition(currTest, inspector);
  }

  gBrowser.removeCurrentTab();
});

function* testPosition(test, inspector) {
  info("Testing " + test.selector);

  let actorID = getHighlighterActorID(inspector.toolbox);

  yield selectAndHighlightNode(test.selector, inspector);

  let {data: position} = yield executeInContent("Test:GetHighlighterAttribute", {
    nodeID: "box-model-nodeinfobar-container",
    name: "position",
    actorID: actorID
  });
  is(position, test.position, "Node " + test.selector + ": position matches");

  let {data: tag} = yield executeInContent("Test:GetHighlighterTextContent", {
    nodeID: "box-model-nodeinfobar-tagname",
    actorID: actorID
  });
  is(tag, test.tag, "node " + test.selector + ": tagName matches.");

  if (test.id) {
    let {data: id} = yield executeInContent("Test:GetHighlighterTextContent", {
      nodeID: "box-model-nodeinfobar-id",
      actorID: actorID
    });
    is(id, "#" + test.id, "node " + test.selector  + ": id matches.");
  }

  let {data: classes} = yield executeInContent("Test:GetHighlighterTextContent", {
    nodeID: "box-model-nodeinfobar-classes",
    actorID: actorID
  });
  is(classes, test.classes, "node " + test.selector  + ": classes match.");

  if (test.dims) {
    let {data: dims} = yield executeInContent("Test:GetHighlighterTextContent", {
      nodeID: "box-model-nodeinfobar-dimensions",
      actorID: actorID
    });
    is(dims, test.dims, "node " + test.selector  + ": dims match.");
  }
}
