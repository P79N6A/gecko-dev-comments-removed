



"use strict";



add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Select the non-animated test node");
  yield selectNode(".still", inspector);

  ok(!panel.toggleAllButtonEl.classList.contains("paused"),
    "The toggle button is in its running state by default");

  info("Toggle all animations, so that they pause");
  yield panel.toggleAll();
  ok(panel.toggleAllButtonEl.classList.contains("paused"),
    "The toggle button now is in its paused state");

  info("Reloading the page");
  let onNewRoot = inspector.once("new-root");
  yield reloadTab();
  yield onNewRoot;
  yield inspector.once("inspector-updated");

  ok(!panel.toggleAllButtonEl.classList.contains("paused"),
    "The toggle button is back in its running state");
});
