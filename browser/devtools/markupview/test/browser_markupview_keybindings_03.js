



"use strict";





const TEST_URL = "data:text/html;charset=utf8,<div></div>Text node";

add_task(function*() {
  let {inspector, toolbox} = yield addTab(TEST_URL).then(openInspector);
  let {walker} = inspector;

  info("Select the test node to have the 2 test containers visible");
  yield selectNode("div", inspector);

  let divFront = yield walker.querySelector(walker.rootNode, "div");
  let textFront = yield walker.nextSibling(divFront);

  info("Click on the MarkupContainer element for the text node");
  yield clickContainer(textFront, inspector);
  is(inspector.markup.doc.activeElement,
     getContainerForNodeFront(textFront, inspector).editor.value,
     "The currently focused element is the node's text content");

  info("Click on the MarkupContainer element for the <div> node");
  yield clickContainer(divFront, inspector);
  is(inspector.markup.doc.activeElement,
     getContainerForNodeFront(divFront, inspector).editor.tag,
     "The currently focused element is the div's tagname");
});
