







function ifWebGLSupported() {
  let { target, panel } = yield initShaderEditor(SIMPLE_CANVAS_URL);
  let { gFront, ShadersEditorsView, EVENTS } = panel.panelWin;

  reload(target);
  yield promise.all([
    once(gFront, "program-linked"),
    once(panel.panelWin, EVENTS.SOURCES_SHOWN)
  ]);

  let vsEditor = yield ShadersEditorsView._getEditor("vs");
  let fsEditor = yield ShadersEditorsView._getEditor("fs");


  is(vsEditor.getText().indexOf("gl_Position"), 170,
    "The vertex shader editor contains the correct text.");
  is(fsEditor.getText().indexOf("gl_FragColor"), 97,
    "The fragment shader editor contains the correct text.");

  yield teardown(panel);
  finish();
}
