







function spawnTest() {
  let { target, panel } = yield initWebAudioEditor(BUFFER_AND_ARRAY_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, EVENTS, InspectorView } = panelWin;
  let gVars = InspectorView._propsView;

  let started = once(gFront, "start-context");

  reload(target);

  let [actors] = yield Promise.all([
    getN(gFront, "create-node", 3),
    waitForGraphRendered(panelWin, 3, 2)
  ]);
  let nodeIds = actors.map(actor => actor.actorID);

  click(panelWin, findGraphNode(panelWin, nodeIds[2]));
  yield once(panelWin, EVENTS.UI_INSPECTOR_NODE_SET);
  checkVariableView(gVars, 0, {
    "curve": "Float32Array"
  }, "WaveShaper's `curve` is listed as an `Float32Array`.");

  click(panelWin, findGraphNode(panelWin, nodeIds[1]));
  yield once(panelWin, EVENTS.UI_INSPECTOR_NODE_SET);
  checkVariableView(gVars, 0, {
    "buffer": "AudioBuffer"
  }, "AudioBufferSourceNode's `buffer` is listed as an `AudioBuffer`.");

  yield teardown(panel);
  finish();
}
