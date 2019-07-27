




"use strict";






const TEST_URI = "data:text/html,<p id='1'>p</p>";

let test = asyncTest(function* () {
  let { inspector, toolbox } = yield openInspectorForURL(TEST_URI);
  yield selectNode("p", inspector);

  let markupLoaded = inspector.once("markuploaded");

  info("Reloading page.");
  content.location.reload();

  info("Waiting for markupview to load after reload.");
  yield markupLoaded;

  is(inspector.selection.node, getNode("p"), "<p> selected after reload.");

  info("Selecting a node to see that inspector still works.");
  yield selectNode("body", inspector);
});
