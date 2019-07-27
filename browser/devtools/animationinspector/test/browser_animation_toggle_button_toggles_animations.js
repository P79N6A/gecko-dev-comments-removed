



"use strict";






add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Click the toggle button");
  yield panel.toggleAll();
  yield checkState("paused");

  info("Click again the toggle button");
  yield panel.toggleAll();
  yield checkState("running");
});

function* checkState(state) {
  for (let selector of [".animated", ".multi", ".long"]) {
    let playState = yield getAnimationPlayerState(selector);
    is(playState, state, "The animation on node " + selector + " is " + state);
  }
}
