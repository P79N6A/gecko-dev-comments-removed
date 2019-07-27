



"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/inspector/" +
                 "test/browser_inspector_infobar_01.html";



let test = asyncTest(function*() {
  info("Loading the test document and opening the inspector");

  yield addTab(TEST_URI);

  let {inspector} = yield openInspector();

  let doc = content.document;
  let testData = [
    {
      node: doc.querySelector("#top"),
      position: "bottom",
      tag: "DIV",
      id: "#top",
      classes: ".class1.class2",
      dims: "500 x 100"
    },
    {
      node: doc.querySelector("#vertical"),
      position: "overlap",
      tag: "DIV",
      id: "#vertical",
      classes: ""
      
    },
    {
      node: doc.querySelector("#bottom"),
      position: "top",
      tag: "DIV",
      id: "#bottom",
      classes: "",
      dims: "500 x 100"
    },
    {
      node: doc.querySelector("body"),
      position: "bottom",
      tag: "BODY",
      id: "",
      classes: ""
      
    },
  ];

  for (let currTest of testData) {
    yield testPosition(currTest, inspector);
  }

  gBrowser.removeCurrentTab();
});

function* testPosition(currTest, inspector) {
  let browser = gBrowser.selectedBrowser;
  let stack = browser.parentNode;

  info("Testing " + currTest.id);

  yield selectAndHighlightNode(currTest.node, inspector);

  let container = stack.querySelector(".highlighter-nodeinfobar-positioner");
  is(container.getAttribute("position"),
    currTest.position, "node " + currTest.id + ": position matches.");

  let tagNameLabel = stack.querySelector(".highlighter-nodeinfobar-tagname");
  is(tagNameLabel.textContent, currTest.tag,
    "node " + currTest.id + ": tagName matches.");

  if (currTest.id) {
    let idLabel = stack.querySelector(".highlighter-nodeinfobar-id");
    is(idLabel.textContent, currTest.id, "node " + currTest.id  + ": id matches.");
  }

  let classesBox = stack.querySelector(".highlighter-nodeinfobar-classes");
  is(classesBox.textContent, currTest.classes,
    "node " + currTest.id  + ": classes match.");

  if (currTest.dims) {
    let dimBox = stack.querySelector(".highlighter-nodeinfobar-dimensions");
    is(dimBox.textContent, currTest.dims, "node " + currTest.id  + ": dims match.");
  }
}
