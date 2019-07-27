







add_task(function*() {
  let { target, panel } = yield initWebAudioEditor(SIMPLE_NODES_URL);
  let { panelWin } = panel;
  let { gFront, $, $$, EVENTS, InspectorView } = panelWin;
  let gVars = InspectorView._propsView;

  let started = once(gFront, "start-context");

  reload(target);

  let [actors] = yield Promise.all([
    getN(gFront, "create-node", 14),
    waitForGraphRendered(panelWin, 14, 0)
  ]);
  let nodeIds = actors.map(actor => actor.actorID);
  let types = [
    "AudioDestinationNode", "AudioBufferSourceNode", "ScriptProcessorNode",
    "AnalyserNode", "GainNode", "DelayNode", "BiquadFilterNode", "WaveShaperNode",
    "PannerNode", "ConvolverNode", "ChannelSplitterNode", "ChannelMergerNode",
    "DynamicsCompressorNode", "OscillatorNode"
  ];

  for (let i = 0; i < types.length; i++) {
    click(panelWin, findGraphNode(panelWin, nodeIds[i]));
    yield once(panelWin, EVENTS.UI_INSPECTOR_NODE_SET);
    checkVariableView(gVars, 0, NODE_DEFAULT_VALUES[types[i]], types[i]);
  }

  yield teardown(target);
});
