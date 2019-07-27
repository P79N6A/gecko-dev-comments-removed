






function ifWebGLSupported() {
  let { target, front } = yield initBackend(MULTIPLE_CONTEXTS_URL);
  front.setup({ reload: true });

  yield getPrograms(front, 2);

  
  yield front.waitForFrame();

  let pixel = yield front.getPixel({ selector: "#canvas1", position: { x: 0, y: 0 }});
  is(pixel.r, 255, "correct `r` value for first canvas.")
  is(pixel.g, 255, "correct `g` value for first canvas.")
  is(pixel.b, 0, "correct `b` value for first canvas.")
  is(pixel.a, 255, "correct `a` value for first canvas.")

  pixel = yield front.getPixel({ selector: "#canvas2", position: { x: 0, y: 0 }});
  is(pixel.r, 0, "correct `r` value for second canvas.")
  is(pixel.g, 255, "correct `g` value for second canvas.")
  is(pixel.b, 255, "correct `b` value for second canvas.")
  is(pixel.a, 255, "correct `a` value for second canvas.")

  yield removeTab(target.tab);
  finish();
}
