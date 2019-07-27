



"use strict";



const TEST_URL = "data:text/html;charset=utf-8," +
                 "<div style='position:absolute;left: 0; top: 0; width: 40000px; height: 8000px'></div>"

const ID = "rulers-highlighter-";

add_task(function*() {
  let { inspector, toolbox, testActor } = yield openInspectorForURL(TEST_URL);
  let front = inspector.inspector;

  let highlighter = yield front.getHighlighterByType("RulersHighlighter");

  
  
  let body = yield getNodeFront("body", inspector);
  yield highlighter.show(body);

  yield isUpdatedAfterScroll(highlighter, inspector, testActor);

  yield highlighter.finalize();
});

function* isUpdatedAfterScroll(highlighterFront, inspector, testActor) {
  info("Checking the rulers' position by default");

  let xAxisRulerTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}x-axis-ruler`, "transform", highlighterFront);
  let xAxisTextTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}x-axis-text`, "transform", highlighterFront);
  let yAxisRulerTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}y-axis-ruler`, "transform", highlighterFront);
  let yAxisTextTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}y-axis-text`, "transform", highlighterFront);

  is(xAxisRulerTransform, null, "x axis ruler is positioned properly");
  is(xAxisTextTransform, null, "x axis text are positioned properly");
  is(yAxisRulerTransform, null, "y axis ruler is positioned properly");
  is(yAxisRulerTransform, null, "y axis text are positioned properly");

  info("Asking the content window to scroll to specific coords");

  let x = 200, y = 300;

  let data = yield testActor.scrollWindow(x, y);

  is(data.x, x, "window scrolled properly horizontally");
  is(data.y, y, "window scrolled properly vertically");

  info("Checking the rulers are properly positioned after the scrolling");

  xAxisRulerTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}x-axis-ruler`, "transform", highlighterFront);
  xAxisTextTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}x-axis-text`, "transform", highlighterFront);
  yAxisRulerTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}y-axis-ruler`, "transform", highlighterFront);
  yAxisTextTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}y-axis-text`, "transform", highlighterFront);

  is(xAxisRulerTransform, `translate(-${x})`, "x axis ruler is positioned properly");
  is(xAxisTextTransform, `translate(-${x})`, "x axis text are positioned properly");
  is(yAxisRulerTransform, `translate(0, -${y})`, "y axis ruler is positioned properly");
  is(yAxisRulerTransform, `translate(0, -${y})`, "y axis text are positioned properly");

  info("Asking the content window to scroll relative to the current position");

  data = yield testActor.scrollWindow(-50, -60, true);

  is(data.x, x - 50, "window scrolled properly horizontally");
  is(data.y, y - 60, "window scrolled properly vertically");

  info("Checking the rulers are properly positioned after the relative scrolling");

  xAxisRulerTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}x-axis-ruler`, "transform", highlighterFront);
  xAxisTextTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}x-axis-text`, "transform", highlighterFront);
  yAxisRulerTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}y-axis-ruler`, "transform", highlighterFront);
  yAxisTextTransform = yield testActor.getHighlighterNodeAttribute(
    `${ID}y-axis-text`, "transform", highlighterFront);

  is(xAxisRulerTransform, `translate(-${x - 50})`, "x axis ruler is positioned properly");
  is(xAxisTextTransform, `translate(-${x - 50})`, "x axis text are positioned properly");
  is(yAxisRulerTransform, `translate(0, -${y - 60})`, "y axis ruler is positioned properly");
  is(yAxisRulerTransform, `translate(0, -${y - 60})`, "y axis text are positioned properly");
}
