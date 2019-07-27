







function* ifTestingSupported() {
  let { target, front } = yield initCallWatcherBackend(SIMPLE_CANVAS_URL);

  ok(target, "Should have a target available.");
  ok(front, "Should have a protocol front available.");

  yield removeTab(target.tab);
  finish();
}
