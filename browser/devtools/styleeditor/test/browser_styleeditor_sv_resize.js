





const TESTCASE_URI = TEST_BASE_HTTP + "simple.html";

add_task(function* () {
  let { toolbox, ui } = yield openStyleEditorForURL(TESTCASE_URI);

  is(ui.editors.length, 2, "There are 2 style sheets initially");

  info("Changing toolbox host to a window.");
  yield toolbox.switchHost(devtools.Toolbox.HostType.WINDOW);

  let editor = yield ui.editors[0].getSourceEditor();
  let originalSourceEditor = editor.sourceEditor;

  let hostWindow = toolbox._host._window;
  let originalWidth = hostWindow.outerWidth;
  let originalHeight = hostWindow.outerHeight;

  
  originalSourceEditor.setCursor(originalSourceEditor.getPosition(4));

  info("Resizing window.");
  hostWindow.resizeTo(120, 480);

  let sourceEditor = ui.editors[0].sourceEditor;
  is(sourceEditor, originalSourceEditor,
     "the editor still references the same Editor instance");

  is(sourceEditor.getOffset(sourceEditor.getCursor()), 4,
     "the caret position has been preserved");

  info("Restoring window to original size.");
  hostWindow.resizeTo(originalWidth, originalHeight);
});

registerCleanupFunction(() => {
  
  Services.prefs.clearUserPref("devtools.toolbox.host");
});
