





waitForExplicitFinish();

const TEST_URL = TEST_BASE + "doc_xulpage.xul";

add_task(function*() {
  let tab = yield addTab(TEST_URL);
  let target = TargetFactory.forTab(tab);

  let toolbox = yield gDevTools.showToolbox(target, "styleeditor");
  let panel = toolbox.getCurrentPanel();
  yield panel.UI.once("editor-added");

  ok(panel, "The style-editor panel did initialize correctly for the XUL window");
});
