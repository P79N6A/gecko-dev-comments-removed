







function ifWebGLSupported() {
  let { target, front } = yield initBackend(SIMPLE_CANVAS_URL);

  let linked = once(front, "program-linked");
  front.setup({ reload: true });
  yield linked;
  ok(true, "Canvas was correctly instrumented on the first navigation.");

  once(front, "program-linked").then(() => {
    ok(false, "A 'program-linked' notification shouldn't have been sent!");
  });

  yield front.finalize();
  yield reload(target);
  yield removeTab(target.tab);
  finish();
}
