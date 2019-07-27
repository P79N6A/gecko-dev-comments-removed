


"use strict";



const TEST_COLOR = "#123ABC";
const COLOR_SELECTOR = "span[data-color]";

add_task(function* () {
  
  requestLongerTimeout(2);

  const TEST_DOC = '<html>                                              \
                      <body>                                            \
                        <div style="color: ' + TEST_COLOR + ';          \
                                    margin: 0px;                        \
                                    background: ' + TEST_COLOR + ';">   \
                          Test "Copy color" context menu option         \
                        </div>                                          \
                      </body>                                           \
                    </html>';

  const TEST_CASES = [
    {
      viewName: "RuleView",
      initializer: openRuleView
    },
    {
      viewName: "ComputedView",
      initializer: openComputedView
    }
  ];

  yield addTab("data:text/html;charset=utf8," + encodeURIComponent(TEST_DOC));

  for (let test of TEST_CASES) {
    yield testView(test);
  }
});

function* testView({viewName, initializer}) {
  info("Testing " + viewName);

  let {inspector, view} = yield initializer();
  yield selectNode("div", inspector);

  testIsColorValueNode(view);
  testIsColorPopupOnAllNodes(view);
  yield clearCurrentNodeSelection(inspector);
}





function testIsColorValueNode(view) {
  info("Testing that child nodes of color nodes are detected.");
  let root = rootElement(view);
  let colorNode = root.querySelector(COLOR_SELECTOR);

  ok(colorNode, "Color node found");
  for (let node of iterateNodes(colorNode)) {
    ok(isColorValueNode(node), "Node is part of color value.");
  }
}





function testIsColorPopupOnAllNodes(view) {
  let root = rootElement(view);
  for (let node of iterateNodes(root)) {
    testIsColorPopupOnNode(view, node);
  }
}








function testIsColorPopupOnNode(view, node) {
  info("Testing node " + node);
  if (view.doc) {
    view.doc.popupNode = node;
  }
  else {
    view.popupNode = node;
  }
  view._colorToCopy = "";

  let result = view._isColorPopup();
  let correct = isColorValueNode(node);

  is(result, correct, "_isColorPopup returned the expected value " + correct);
  is(view._colorToCopy, (correct) ? TEST_COLOR : "",
     "_colorToCopy was set to the expected value");
}





function isColorValueNode(node) {
  let container = (node.nodeType == node.TEXT_NODE) ?
                   node.parentElement : node;

  let isColorNode = el => el.dataset && "color" in el.dataset;

  while (!isColorNode(container)) {
    container = container.parentNode;
    if (!container) {
      info("No color. Node is not part of color value.");
      return false;
    }
  }

  info("Found a color. Node is part of color value.");

  return true;
}




function* iterateNodes(baseNode) {
  yield baseNode;

  for (let child of baseNode.childNodes) {
    yield* iterateNodes(child);
  }
}




let rootElement = view => (view.element) ? view.element : view.styleDocument;
