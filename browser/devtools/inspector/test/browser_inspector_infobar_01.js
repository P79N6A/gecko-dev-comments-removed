



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
      dims: "500" + " \u00D7 " + "100"
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
      dims: "500" + " \u00D7 " + "100"
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
});

function* testPosition(test, inspector) {
  info("Testing " + test.selector);

  yield selectAndHighlightNode(test.selector, inspector);

  let highlighter = inspector.toolbox.highlighter;
  let position = yield getHighlighterNodeAttribute(highlighter,
    "box-model-nodeinfobar-container", "position");
  is(position, test.position, "Node " + test.selector + ": position matches");

  let tag = yield getHighlighterNodeTextContent(highlighter,
    "box-model-nodeinfobar-tagname");
  is(tag, test.tag, "node " + test.selector + ": tagName matches.");

  if (test.id) {
    let id = yield getHighlighterNodeTextContent(highlighter,
      "box-model-nodeinfobar-id");
    is(id, "#" + test.id, "node " + test.selector  + ": id matches.");
  }

  let classes = yield getHighlighterNodeTextContent(highlighter,
    "box-model-nodeinfobar-classes");
  is(classes, test.classes, "node " + test.selector  + ": classes match.");

  if (test.dims) {
    let dims = yield getHighlighterNodeTextContent(highlighter,
      "box-model-nodeinfobar-dimensions");
    is(dims, test.dims, "node " + test.selector  + ": dims match.");
  }
}
