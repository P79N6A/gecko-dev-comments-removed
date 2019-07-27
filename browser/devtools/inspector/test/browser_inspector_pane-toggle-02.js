


"use strict";





add_task(function* () {
  info("Open the inspector in a side toolbox host");
  let {toolbox, inspector} = yield openInspectorForURL("about:blank", "side");

  let button = inspector.panelDoc.getElementById("inspector-pane-toggle");
  ok(button, "The toggle button exists in the DOM");
  is(button.parentNode.id, "inspector-toolbar", "The toggle button is in the toolbar");
  ok(!button.hasAttribute("pane-collapsed"), "The button is in expanded state");
  ok(!button.getClientRects().length, "The button is hidden");

  info("Switch the host to bottom type");
  yield toolbox.switchHost("bottom");

  ok(!!button.getClientRects().length, "The button is visible");
});
