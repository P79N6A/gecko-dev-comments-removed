



"use strict";





let L10N = new ViewHelpers.L10N();

add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Start an animation on the test node");
  yield executeInContent("devtools:test:setAttribute", {
    selector: ".still",
    attributeName: "class",
    attributeValue: "ball still short"
  });

  info("Select the node");
  yield selectNode(".still", inspector);

  info("Wait until the animation ends");
  let widget = panel.playerWidgets[0];
  let front = widget.player;

  yield waitForStateCondition(front, state => state.playState === "finished",
                              "Waiting for the animation to finish");

  is(widget.currentTimeEl.value, front.state.duration,
    "The timeline slider has the right value");
  is(widget.timeDisplayEl.textContent,
    L10N.numberWithDecimals(front.state.duration / 1000, 2) + "s",
    "The timeline slider has the right value");
});
