







function ifWebGLSupported() {
  let [target, debuggee, panel] = yield initShaderEditor(MULTIPLE_CONTEXTS_URL);
  let { gFront, EVENTS, L10N, ShadersListView, ShadersEditorsView } = panel.panelWin;

  is(ShadersListView.itemCount, 0,
    "The shaders list should initially be empty.");
  is(ShadersListView.selectedItem, null,
    "The shaders list has no selected item.");
  is(ShadersListView.selectedIndex, -1,
    "The shaders list has a negative index.");

  reload(target);

  let firstProgramActor = yield once(gFront, "program-linked");

  is(ShadersListView.itemCount, 1,
    "The shaders list contains one entry.");
  is(ShadersListView.selectedItem, ShadersListView.items[0],
    "The shaders list has a correct item selected.");
  is(ShadersListView.selectedIndex, 0,
    "The shaders list has a correct index selected.");

  let secondProgramActor = yield once(gFront, "program-linked");

  is(ShadersListView.itemCount, 2,
    "The shaders list contains two entries.");
  is(ShadersListView.selectedItem, ShadersListView.items[0],
    "The shaders list has a correct item selected.");
  is(ShadersListView.selectedIndex, 0,
    "The shaders list has a correct index selected.");

  is(ShadersListView.labels[0], L10N.getFormatStr("shadersList.programLabel", 0),
    "The correct first label is shown in the shaders list.");
  is(ShadersListView.labels[1], L10N.getFormatStr("shadersList.programLabel", 1),
    "The correct second label is shown in the shaders list.");

  let vertexShader = yield firstProgramActor.getVertexShader();
  let fragmentShader = yield firstProgramActor.getFragmentShader();
  let vertSource = yield vertexShader.getText();
  let fragSource = yield fragmentShader.getText();

  let vsEditor = yield ShadersEditorsView._getEditor("vs");
  let fsEditor = yield ShadersEditorsView._getEditor("fs");

  is(vertSource, vsEditor.getText(),
    "The vertex shader editor contains the correct text.");
  is(fragSource, fsEditor.getText(),
    "The vertex shader editor contains the correct text.");

  let compiled = once(panel.panelWin, EVENTS.SHADER_COMPILED).then(() => {
    ok(false, "Selecting a different program shouldn't recompile its shaders.");
  });

  let shown = once(panel.panelWin, EVENTS.SOURCES_SHOWN).then(() => {
    ok(true, "The vertex and fragment sources have changed in the editors.");
  });

  EventUtils.sendMouseEvent({ type: "mousedown" }, ShadersListView.items[1].target);
  yield shown;

  is(ShadersListView.selectedItem, ShadersListView.items[1],
    "The shaders list has a correct item selected.");
  is(ShadersListView.selectedIndex, 1,
    "The shaders list has a correct index selected.");

  yield teardown(panel);
  finish();
}

function once(aTarget, aEvent) {
  let deferred = promise.defer();
  aTarget.once(aEvent, deferred.resolve);
  return deferred.promise;
}
