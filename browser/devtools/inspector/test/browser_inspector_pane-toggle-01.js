


"use strict";




add_task(function* () {
  info("Open the inspector in a bottom toolbox host");
  let {toolbox, inspector} = yield openInspectorForURL("about:blank", "bottom");

  let button = inspector.panelDoc.getElementById("inspector-pane-toggle");
  ok(button, "The toggle button exists in the DOM");
  is(button.parentNode.id, "inspector-toolbar", "The toggle button is in the toolbar");
  ok(!button.hasAttribute("pane-collapsed"), "The button is in expanded state");
  ok(!!button.getClientRects().length, "The button is visible");

  info("Switch the host to side type");
  yield toolbox.switchHost("side");

  ok(!button.getClientRects().length, "The button is hidden");
});
