



"use strict";




const TEST_URL = "data:text/html;charset=utf8,<div></div>";

add_task(function*() {
  let {inspector, toolbox} = yield addTab(TEST_URL).then(openInspector);

  info("Focusing the tag editor of the test element");
  let {editor} = yield getContainerForSelector("div", inspector);
  editor.tag.focus();

  info("Pressing ESC and wait for the split-console to open");
  let onSplitConsole = toolbox.once("split-console");
  EventUtils.synthesizeKey("VK_ESCAPE", {}, inspector.panelWin);
  yield onSplitConsole;
  ok(toolbox.splitConsole, "The split console is shown.");

  info("Pressing ESC again and wait for the split-console to close");
  onSplitConsole = toolbox.once("split-console");
  EventUtils.synthesizeKey("VK_ESCAPE", {}, inspector.panelWin);
  yield onSplitConsole;
  ok(!toolbox.splitConsole, "The split console is hidden.");
});
