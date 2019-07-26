






function ifWebGLSupported() {
  let [target, debuggee, panel] = yield initShaderEditor(MULTIPLE_CONTEXTS_URL);
  let { gFront, ShadersListView, ShadersEditorsView } = panel.panelWin;

  reload(target);
  let programActor = yield once(gFront, "program-linked");
  let programItem = ShadersListView.selectedItem;

  is(programItem.attachment.programActor, programActor,
    "The correct program actor is cached for the selected item.");

  is((yield programActor.getVertexShader()),
     (yield programItem.attachment.vs),
    "The cached vertex shader promise returns the correct actor.");

  is((yield programActor.getFragmentShader()),
     (yield programItem.attachment.fs),
    "The cached fragment shader promise returns the correct actor.");

  is((yield (yield programActor.getVertexShader()).getText()),
     (yield (yield ShadersEditorsView._getEditor("vs")).getText()),
    "The cached vertex shader promise returns the correct text.");

  is((yield (yield programActor.getFragmentShader()).getText()),
     (yield (yield ShadersEditorsView._getEditor("fs")).getText()),
    "The cached fragment shader promise returns the correct text.");

  yield teardown(panel);
  finish();
}
