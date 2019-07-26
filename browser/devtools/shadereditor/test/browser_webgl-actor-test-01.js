






function ifWebGLSupported() {
  let [target, debuggee, front] = yield initBackend(SIMPLE_CANVAS_URL);

  ok(target, "Should have a target available.");
  ok(debuggee, "Should have a debuggee available.");
  ok(front, "Should have a protocol front available.");

  yield removeTab(target.tab);
  finish();
}
