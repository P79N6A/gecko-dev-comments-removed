



"use strict";




const TEST_URI = "data:text/html;charset=utf-8," +
  "<p>testing the highlighter goes away on destroy</p>";

add_task(function* () {
  let { inspector, toolbox } = yield openInspectorForURL(TEST_URI);
  let pickerStopped = toolbox.once("picker-stopped");

  yield selectNode("p", inspector);

  info("Inspector displayed and ready, starting the picker.");
  yield toolbox.highlighterUtils.startPicker();

  info("Destroying the toolbox.");
  yield toolbox.destroy();

  info("Waiting for the picker-stopped event that should be fired when the " +
       "toolbox is destroyed.");
  yield pickerStopped;

  ok(true, "picker-stopped event fired after switch tools, so picker is closed.");
});
