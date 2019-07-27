




"use strict";






const TEST_URI = "data:text/html,<p id='1'>p</p>";

add_task(function* () {
  let { inspector, testActor } = yield openInspectorForURL(TEST_URI);
  yield selectNode("p", inspector);

  let markupLoaded = inspector.once("markuploaded");

  info("Reloading page.");
  yield testActor.eval("location.reload()");

  info("Waiting for markupview to load after reload.");
  yield markupLoaded;

  let nodeFront = yield getNodeFront("p", inspector);
  is(inspector.selection.nodeFront, nodeFront, "<p> selected after reload.");

  info("Selecting a node to see that inspector still works.");
  yield selectNode("body", inspector);
});
