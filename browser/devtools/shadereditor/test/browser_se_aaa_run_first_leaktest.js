







function ifWebGLSupported() {
  let [target, debuggee, panel] = yield initShaderEditor(SIMPLE_CANVAS_URL);

  ok(target, "Should have a target available.");
  ok(debuggee, "Should have a debuggee available.");
  ok(panel, "Should have a panel available.");

  yield teardown(panel);
  finish();
}
