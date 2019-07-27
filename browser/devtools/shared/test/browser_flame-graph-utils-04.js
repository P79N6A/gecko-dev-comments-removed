




let {FlameGraphUtils, FLAME_GRAPH_BLOCK_HEIGHT} = devtools.require("devtools/shared/widgets/FlameGraph");
let {FrameNode} = devtools.require("devtools/shared/profiler/tree-model");

add_task(function*() {
  yield promiseTab("about:blank");
  yield performTest();
  gBrowser.removeCurrentTab();
});

function* performTest() {
  let out = FlameGraphUtils.createFlameGraphDataFromThread(TEST_DATA, {
    flattenRecursion: true,
    contentOnly: true,
    showIdleBlocks: '\m/'
  });

  ok(out, "Some data was outputted properly");
  is(out.length, 10, "The outputted length is correct.");

  info("Got flame graph data:\n" + out.toSource() + "\n");

  for (let i = 0; i < out.length; i++) {
    let found = out[i];
    let expected = EXPECTED_OUTPUT[i];

    is(found.blocks.length, expected.blocks.length,
      "The correct number of blocks were found in this bucket.");

    for (let j = 0; j < found.blocks.length; j++) {
      is(found.blocks[j].x, expected.blocks[j].x,
        "The expected block X position is correct for this frame.");
      is(found.blocks[j].y, expected.blocks[j].y,
        "The expected block Y position is correct for this frame.");
      is(found.blocks[j].width, expected.blocks[j].width,
        "The expected block width is correct for this frame.");
      is(found.blocks[j].height, expected.blocks[j].height,
        "The expected block height is correct for this frame.");
      is(found.blocks[j].text, expected.blocks[j].text,
        "The expected block text is correct for this frame.");
    }
  }
}

let TEST_DATA = synthesizeProfileForTest([{
  frames: [{
    location: "http://A"
  }, {
    location: "http://A"
  }, {
    location: "http://A"
  }, {
    location: "https://B"
  }, {
    location: "https://B"
  }, {
    location: "file://C",
  }, {
    location: "chrome://D"
  }, {
    location: "resource://E"
  }],
  time: 50
}, {
  frames: [],
  time: 100
}, {
  frames: [{
    location: "http://A"
  }, {
    location: "https://B"
  }, {
    location: "file://C",
  }],
  time: 150
}]);

let EXPECTED_OUTPUT = [{
  blocks: []
}, {
  blocks: []
}, {
  blocks: [{
    srcData: {
      startTime: 0,
      rawLocation: "http://A"
    },
    x: 0,
    y: 0,
    width: 50,
    height: FLAME_GRAPH_BLOCK_HEIGHT,
    text: "http://A"
  }, {
    srcData: {
      startTime: 0,
      rawLocation: "file://C"
    },
    x: 0,
    y: FLAME_GRAPH_BLOCK_HEIGHT * 2,
    width: 50,
    height: FLAME_GRAPH_BLOCK_HEIGHT,
    text: "file://C"
  }, {
    srcData: {
      startTime: 100,
      rawLocation: "http://A"
    },
    x: 100,
    y: 0,
    width: 50,
    height: FLAME_GRAPH_BLOCK_HEIGHT,
    text: "http://A"
  }, {
    srcData: {
      startTime: 0,
      rawLocation: "file://C"
    },
    x: 100,
    y: FLAME_GRAPH_BLOCK_HEIGHT * 2,
    width: 50,
    height: FLAME_GRAPH_BLOCK_HEIGHT,
    text: "file://C"
  }]
}, {
  blocks: [{
    srcData: {
      startTime: 50,
      rawLocation: "\m/"
    },
    x: 50,
    y: 0,
    width: 50,
    height: FLAME_GRAPH_BLOCK_HEIGHT,
    text: "\m/"
  }]
}, {
  blocks: []
}, {
  blocks: []
}, {
  blocks: [{
    srcData: {
      startTime: 0,
      rawLocation: "Gecko"
    },
    x: 0,
    y: FLAME_GRAPH_BLOCK_HEIGHT * 3,
    width: 50,
    height: FLAME_GRAPH_BLOCK_HEIGHT,
    text: "Gecko"
  }]
}, {
  blocks: []
}, {
  blocks: [{
    srcData: {
      startTime: 0,
      rawLocation: "https://B"
    },
    x: 0,
    y: FLAME_GRAPH_BLOCK_HEIGHT,
    width: 50,
    height: FLAME_GRAPH_BLOCK_HEIGHT,
    text: "https://B"
  }, {
    srcData: {
      startTime: 100,
      rawLocation: "https://B"
    },
    x: 100,
    y: FLAME_GRAPH_BLOCK_HEIGHT,
    width: 50,
    height: FLAME_GRAPH_BLOCK_HEIGHT,
    text: "https://B"
  }]
}, {
  blocks: []
}];
