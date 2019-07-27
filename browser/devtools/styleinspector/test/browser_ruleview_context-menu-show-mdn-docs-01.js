















"use strict";

const {setBaseCssDocsUrl} = devtools.require("devtools/shared/widgets/MdnDocsWidget");






const TEST_DOC =`
<html>
  <head>
    <style>
      padding {font-family: margin;}
    </style>
  </head>

  <body>
    <padding>MDN tooltip testing</padding>
  </body>
</html>`;

add_task(function* () {

  yield addTab("data:text/html;charset=utf8," + encodeURIComponent(TEST_DOC));

  let {inspector, view} = yield openRuleView();
  yield selectNode("padding", inspector);

  yield testMdnContextMenuItemVisibility(view);
});











function* testMdnContextMenuItemVisibility(view) {
  info("Test that MDN context menu item is shown only when it should be.");

  let root = rootElement(view);
  for (let node of iterateNodes(root)) {
    info("Setting " + node + " as popupNode");
    view.doc.popupNode = node;

    info("Updating context menu state");
    view._contextMenuUpdate();
    let isVisible = !view.menuitemShowMdnDocs.hidden;
    let shouldBeVisible = isPropertyNameNode(node);
    let message = shouldBeVisible? "shown": "hidden";
    is(isVisible, shouldBeVisible,
       "The MDN context menu item is " + message);
  }
}




function isPropertyNameNode(node) {
  return ((node.nodeType === node.TEXT_NODE) &&
          (node.textContent === "font-family"));
}




function* iterateNodes(baseNode) {
  yield baseNode;

  for (let child of baseNode.childNodes) {
    yield* iterateNodes(child);
  }
}




let rootElement = view => (view.element) ? view.element : view.styleDocument;
