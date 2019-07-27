







thisTestLeaksUncaughtRejectionsAndShouldBeFixed("Error: Shader Editor is still waiting for a WebGL context to be created.");






function ifWebGLSupported() {
  let { target, panel } = yield initShaderEditor(SIMPLE_CANVAS_URL);

  ok(target, "Should have a target available.");
  ok(panel, "Should have a panel available.");

  yield teardown(panel);
  finish();
}
