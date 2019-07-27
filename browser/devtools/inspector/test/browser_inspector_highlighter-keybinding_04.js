



"use strict";




const TEST_URL = "data:text/html;charset=utf8,<div></div>";

add_task(function*() {
  let {inspector, toolbox, testActor} = yield openInspectorForURL(TEST_URL);

  info("Start the element picker");
  yield toolbox.highlighterUtils.startPicker();

  info("Start using the picker by hovering over nodes");
  let onHover = toolbox.once("picker-node-hovered");
  testActor.synthesizeMouse({
    options: {type: "mousemove"},
    center: true,
    selector: "div"
  });
  yield onHover;

  info("Press escape and wait for the picker to stop");
  let onPickerStopped = toolbox.once("picker-stopped");
  testActor.synthesizeKey({
    key: "VK_ESCAPE",
    options: {}
  });
  yield onPickerStopped;

  info("Press escape again and wait for the split console to open");
  let onSplitConsole = toolbox.once("split-console");
  
  
  EventUtils.synthesizeKey("VK_ESCAPE", {}, inspector.panelWin);
  yield onSplitConsole;
  ok(toolbox.splitConsole, "The split console is shown.");

  
  yield toolbox.toggleSplitConsole();
});
