






add_task(function*() {
  let { target, front } = yield initBackend(SIMPLE_NODES_URL);
  let [_, nodes] = yield Promise.all([
    front.setup({ reload: true }),
    getN(front, "create-node", 15)
  ]);

  let allNodeParams = yield Promise.all(nodes.map(node => node.getParams()));
  let nodeTypes = [
    "AudioDestinationNode",
    "AudioBufferSourceNode", "ScriptProcessorNode", "AnalyserNode", "GainNode",
    "DelayNode", "BiquadFilterNode", "WaveShaperNode", "PannerNode", "ConvolverNode",
    "ChannelSplitterNode", "ChannelMergerNode", "DynamicsCompressorNode", "OscillatorNode",
    "StereoPannerNode"
  ];

  nodeTypes.forEach((type, i) => {
    let params = allNodeParams[i];
    params.forEach(({param, value, flags}) => {
      ok(param in NODE_DEFAULT_VALUES[type], "expected parameter for " + type);

      ok(typeof flags === "object", type + " has a flags object");

      if (param === "buffer") {
        is(flags.Buffer, true, "`buffer` params have Buffer flag");
      }
      else if (param === "bufferSize" || param === "frequencyBinCount") {
        is(flags.readonly, true, param + " is readonly");
      }
      else if (param === "curve") {
        is(flags["Float32Array"], true, "`curve` param has Float32Array flag");
      } else {
        is(Object.keys(flags), 0, type + "-" + param + " has no flags set")
      }
    });
  });

  yield removeTab(target.tab);
});
