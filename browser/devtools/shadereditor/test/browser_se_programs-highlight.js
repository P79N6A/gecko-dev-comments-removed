






function ifWebGLSupported() {
  let [target, debuggee, panel] = yield initShaderEditor(MULTIPLE_CONTEXTS_URL);
  let { gFront, EVENTS, ShadersListView, ShadersEditorsView } = panel.panelWin;

  once(panel.panelWin, EVENTS.SHADER_COMPILED).then(() => {
    ok(false, "No shaders should be publicly compiled during this test.");
  });

  reload(target);
  let firstProgramActor = yield once(gFront, "program-linked");
  let secondProgramActor = yield once(gFront, "program-linked");

  let vsEditor = yield ShadersEditorsView._getEditor("vs");
  let fsEditor = yield ShadersEditorsView._getEditor("fs");

  vsEditor.once("change", () => {
    ok(false, "The vertex shader source was unexpectedly changed.");
  });
  fsEditor.once("change", () => {
    ok(false, "The fragment shader source was unexpectedly changed.");
  });
  once(panel.panelWin, EVENTS.SOURCES_SHOWN).then(() => {
    ok(false, "No sources should be changed form this point onward.");
  });

  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 255, g: 255, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 0, g: 255, b: 255, a: 255 }, true, "#canvas2");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 255, g: 255, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 0, g: 255, b: 255, a: 255 }, true, "#canvas2");

  ShadersListView._onShaderMouseEnter({ target: getItemLabel(panel, 0) });

  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 255, g: 0, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 0, g: 255, b: 255, a: 255 }, true, "#canvas2");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 255, g: 0, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 0, g: 255, b: 255, a: 255 }, true, "#canvas2");
  ok(true, "The first program was correctly highlighted.");

  ShadersListView._onShaderMouseLeave({ target: getItemLabel(panel, 0) });
  ShadersListView._onShaderMouseEnter({ target: getItemLabel(panel, 1) });

  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 255, g: 255, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 255, g: 0, b: 0, a: 255 }, true, "#canvas2");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 255, g: 255, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 255, g: 0, b: 0, a: 255 }, true, "#canvas2");
  ok(true, "The second program was correctly highlighted.");

  ShadersListView._onShaderMouseLeave({ target: getItemLabel(panel, 1) });

  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 255, g: 255, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 0, g: 255, b: 255, a: 255 }, true, "#canvas2");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 255, g: 255, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 0, g: 255, b: 255, a: 255 }, true, "#canvas2");
  ok(true, "The two programs were correctly unhighlighted.");

  ShadersListView._onShaderMouseEnter({ target: getBlackBoxCheckbox(panel, 0) });

  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 255, g: 255, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 0, g: 255, b: 255, a: 255 }, true, "#canvas2");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 255, g: 255, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 0, g: 255, b: 255, a: 255 }, true, "#canvas2");
  ok(true, "The two programs were left unchanged after hovering a blackbox checkbox.");

  ShadersListView._onShaderMouseLeave({ target: getBlackBoxCheckbox(panel, 0) });

  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 255, g: 255, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 0, y: 0 }, { r: 0, g: 255, b: 255, a: 255 }, true, "#canvas2");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 255, g: 255, b: 0, a: 255 }, true, "#canvas1");
  yield ensurePixelIs(debuggee, { x: 127, y: 127 }, { r: 0, g: 255, b: 255, a: 255 }, true, "#canvas2");
  ok(true, "The two programs were left unchanged after unhovering a blackbox checkbox.");

  yield teardown(panel);
  finish();
}

function getItemLabel(aPanel, aIndex) {
  return aPanel.panelWin.document.querySelectorAll(
    ".side-menu-widget-item-label")[aIndex];
}

function getBlackBoxCheckbox(aPanel, aIndex) {
  return aPanel.panelWin.document.querySelectorAll(
    ".side-menu-widget-item-checkbox")[aIndex];
}

function once(aTarget, aEvent) {
  let deferred = promise.defer();
  aTarget.once(aEvent, deferred.resolve);
  return deferred.promise;
}
