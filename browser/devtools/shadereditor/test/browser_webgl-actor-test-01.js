






function ifWebGLSupported() {
  let { target, front } = yield initBackend(SIMPLE_CANVAS_URL);

  ok(target, "Should have a target available.");
  ok(front, "Should have a protocol front available.");

  yield removeTab(target.tab);
  finish();
}
