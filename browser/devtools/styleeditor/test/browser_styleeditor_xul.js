





waitForExplicitFinish();

const TEST_URL = "about:config";

add_task(function*() {
  let tab = yield addTab(TEST_URL);
  let target = TargetFactory.forTab(tab);

  let toolbox = yield gDevTools.showToolbox(target, "styleeditor");
  let panel = toolbox.getCurrentPanel();
  yield panel.UI.once("editor-added");

  ok(panel, "The style-editor panel did initialize correctly for the XUL window");
});
