



"use strict";




const FRAME_SRC = "data:text/html;charset=utf-8," +
  "<div class=sub-level-node></div>";

const TEST_URL = "data:text/html;charset=utf-8," +
  "<div class=root-level-node></div>" +
  "<iframe src=\"" + FRAME_SRC + "\" />";

const TEST_DATA = [{
  selector: ".root-level-node",
  containerCount: 1
}, {
  selector: ".sub-level-node",
  containerCount: 0
}, {
  inIframe: true,
  selector: ".root-level-node",
  containerCount: 0
}, {
  inIframe: true,
  selector: ".sub-level-node",
  containerCount: 1
}];

add_task(function*() {
  let {inspector, toolbox} = yield openInspectorForURL(TEST_URL);
  let front = inspector.inspector;
  let highlighter = yield front.getHighlighterByType("SelectorHighlighter");

  for (let {inIframe, selector, containerCount} of TEST_DATA) {
    info("Showing the highlighter on " + selector + ". Expecting " +
      containerCount + " highlighter containers");

    let contextNode;
    if (inIframe) {
      contextNode = yield getNodeFrontInFrame("body", "iframe", inspector);
    } else {
      contextNode = yield getNodeFront("body", inspector);
    }

    yield highlighter.show(contextNode, {selector});

    let {data: nb} = yield executeInContent("Test:GetSelectorHighlighterBoxNb", {
      actorID: highlighter.actorID
    });
    ok(nb !== null, "The number of highlighters was retrieved");

    is(nb, containerCount, "The correct number of highlighers were created");
    yield highlighter.hide();
  }

  yield highlighter.finalize();
  gBrowser.removeCurrentTab();
});
