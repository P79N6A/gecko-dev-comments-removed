


"use strict";




const TEST_URI = "data:text/html;charset=utf-8," +
  "<p>bug 699308 - test iframe navigation</p>" +
  "<iframe src='data:text/html;charset=utf-8,hello world'></iframe>";

add_task(function* () {
  let { inspector, toolbox, testActor } = yield openInspectorForURL(TEST_URI);

  info("Starting element picker.");
  yield toolbox.highlighterUtils.startPicker();

  info("Waiting for highlighter to activate.");
  let highlighterShowing = toolbox.once("highlighter-ready");
  testActor.synthesizeMouse({
    selector: "body",
    options: {type: "mousemove"},
    x: 1,
    y: 1
  });
  yield highlighterShowing;

  let isVisible = yield testActor.isHighlighting();
  ok(isVisible, "Inspector is highlighting.");

  yield testActor.reloadFrame("iframe");
  info("Frame reloaded. Reloading again.");

  yield testActor.reloadFrame("iframe");
  info("Frame reloaded twice.");

  isVisible = yield testActor.isHighlighting();
  ok(isVisible, "Inspector is highlighting after iframe nav.");

  info("Stopping element picker.");
  yield toolbox.highlighterUtils.stopPicker();
});
