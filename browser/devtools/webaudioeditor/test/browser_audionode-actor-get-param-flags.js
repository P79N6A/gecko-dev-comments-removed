






function spawnTest () {
  let { target, front } = yield initBackend(SIMPLE_NODES_URL);
  let [_, nodes] = yield Promise.all([
    front.setup({ reload: true }),
    getN(front, "create-node", 14)
  ]);

  let allNodeParams = yield Promise.all(nodes.map(node => node.getParams()));
  let nodeTypes = [
    "AudioDestinationNode",
    "AudioBufferSourceNode", "ScriptProcessorNode", "AnalyserNode", "GainNode",
    "DelayNode", "BiquadFilterNode", "WaveShaperNode", "PannerNode", "ConvolverNode",
    "ChannelSplitterNode", "ChannelMergerNode", "DynamicsCompressorNode", "OscillatorNode"
  ];

  
  
  for (let i = 0; i < nodeTypes.length; i++) {
    let type = nodeTypes[i];
    let params = allNodeParams[i];

    for (let {param, value, flags} of params) {
      let testFlags = yield nodes[i].getParamFlags(param);
      ok(typeof testFlags === "object", type + " has flags from #getParamFlags(" + param + ")");

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
    }
  }

  yield removeTab(target.tab);
  finish();
}
