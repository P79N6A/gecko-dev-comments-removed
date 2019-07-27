



"use strict";















const TEST_URL = TEST_URL_ROOT + "doc_inspector_highlighter_csstransform.html";

add_task(function*() {
  let {inspector, toolbox} = yield openInspectorForURL(TEST_URL);
  let front = inspector.inspector;

  let highlighter = yield front.getHighlighterByType("CssTransformHighlighter");

  let nodeFront = yield getNodeFront("#test-node", inspector);

  info("Displaying the transform highlighter on test node");
  yield highlighter.show(nodeFront);

  let {data} = yield executeInContent("Test:GetAllAdjustedQuads", {
    selector: "#test-node"
  });
  let [expected] = data.border;

  let points = yield getHighlighterNodeAttribute(highlighter,
    "css-transform-transformed", "points");
  let polygonPoints = points.split(" ").map(p => {
    return {
      x: +p.substring(0, p.indexOf(",")),
      y: +p.substring(p.indexOf(",")+1)
    };
  });

  for (let i = 1; i < 5; i ++) {
    is(polygonPoints[i - 1].x, expected["p" + i].x,
      "p" + i + " x coordinate is correct");
    is(polygonPoints[i - 1].y, expected["p" + i].y,
      "p" + i + " y coordinate is correct");
  }

  info("Hiding the transform highlighter");
  yield highlighter.hide();
  yield highlighter.finalize();
});
