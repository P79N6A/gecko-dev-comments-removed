







function ifWebGLSupported() {
  let [target, debuggee, front] = yield initBackend(SIMPLE_CANVAS_URL);
  front.setup({ reload: true });

  let programActor = yield once(front, "program-linked");
  let vertexShader = yield programActor.getVertexShader();
  let fragmentShader = yield programActor.getFragmentShader();

  let vertSource = yield vertexShader.getText();
  ok(vertSource.contains("gl_Position"),
    "The correct vertex shader source was retrieved.");

  let fragSource = yield fragmentShader.getText();
  ok(fragSource.contains("gl_FragColor"),
    "The correct fragment shader source was retrieved.");

  yield removeTab(target.tab);
  finish();
}
