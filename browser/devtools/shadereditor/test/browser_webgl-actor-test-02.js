







function* ifWebGLSupported() {
  let { target, front } = yield initBackend(SIMPLE_CANVAS_URL);

  once(front, "program-linked").then(() => {
    ok(false, "A 'program-linked' notification shouldn't have been sent!");
  });

  ok(true, "Each test requires at least one pass, fail or todo so here is a pass.");

  yield reload(target);
  yield removeTab(target.tab);
  finish();
}
