



"use strict";



add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel, controller} = yield openAnimationInspector();

  info("Apply 2 finite animations to the test node");
  yield executeInContent("devtools:test:setAttribute", {
    selector: ".still",
    attributeName: "class",
    attributeValue: "ball still multi-finite"
  });

  info("Select the test node");
  yield selectNode(".still", inspector);

  is(controller.animationPlayers.length, 2, "2 animation players exist");

  info("Wait for both animations to end");
  let promises = controller.animationPlayers.map(front => {
    return waitForStateCondition(front, state => state.playState === "finished",
                                 "Waiting for the animation to finish");
  });
  yield promise.all(promises);

  for (let widgetEl of panel.playersEl.querySelectorAll(".player-widget")) {
    ok(widgetEl.classList.contains("finished"), "The player widget has the right class");
  }
});
