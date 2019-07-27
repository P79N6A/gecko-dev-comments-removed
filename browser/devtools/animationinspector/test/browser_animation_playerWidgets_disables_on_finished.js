



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
    let def = promise.defer();
    let onStateChanged = () => {
      if (front.state.playState === "finished") {
        front.off(front.AUTO_REFRESH_EVENT, onStateChanged);
        def.resolve();
      }
    };
    front.on(front.AUTO_REFRESH_EVENT, onStateChanged);
    return def.promise;
  });

  yield promise.all(promises);

  for (let widgetEl of panel.playersEl.querySelectorAll(".player-widget")) {
    ok(widgetEl.classList.contains("finished"), "The player widget has the right class");
  }
});
