




let {FlameGraphUtils} = Cu.import("resource:///modules/devtools/FlameGraph.jsm", {});

let test = Task.async(function*() {
  yield promiseTab("about:blank");
  yield performTest();
  gBrowser.removeCurrentTab();
  finish();
});

function* performTest() {
  let out = FlameGraphUtils.createFlameGraphDataFromSamples(TEST_DATA);

  ok(out, "Some data was outputted properly");
  is(out.length, 10, "The outputted length is correct.");

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

let TEST_DATA = [{
  frames: [{
    location: "M"
  }, {
    location: "N",
  }, {
    location: "P"
  }],
  time: 50,
}, {
  frames: [{
    location: "A"
  }, {
    location: "B",
  }, {
    location: "C"
  }],
  time: 100,
}, {
  frames: [{
    location: "A"
  }, {
    location: "B",
  }, {
    location: "D"
  }],
  time: 210,
}, {
  frames: [{
    location: "A"
  }, {
    location: "E",
  }, {
    location: "F"
  }],
  time: 330,
}, {
  frames: [{
    location: "A"
  }, {
    location: "B",
  }, {
    location: "C"
  }],
  time: 460,
}, {
  frames: [{
    location: "X"
  }, {
    location: "Y",
  }, {
    location: "Z"
  }],
  time: 500
}];

let EXPECTED_OUTPUT = [{
  blocks: [{
    x: 0,
    y: 24,
    width: 50,
    height: 12,
    text: "P"
  }, {
    x: 210,
    y: 24,
    width: 120,
    height: 12,
    text: "F"
  }, {
    x: 460,
    y: 24,
    width: 40,
    height: 12,
    text: "Z"
  }]
}, {
  blocks: []
}, {
  blocks: []
}, {
  blocks: []
}, {
  blocks: []
}, {
  blocks: [{
    x: 50,
    y: 0,
    width: 410,
    height: 12,
    text: "A"
  }]
}, {
  blocks: [{
    x: 50,
    y: 12,
    width: 160,
    height: 12,
    text: "B"
  }, {
    x: 330,
    y: 12,
    width: 130,
    height: 12,
    text: "B"
  }]
}, {
  blocks: [{
    x: 0,
    y: 0,
    width: 50,
    height: 12,
    text: "M"
  }, {
    x: 50,
    y: 24,
    width: 50,
    height: 12,
    text: "C"
  }, {
    x: 330,
    y: 24,
    width: 130,
    height: 12,
    text: "C"
  }]
}, {
  blocks: [{
    x: 0,
    y: 12,
    width: 50,
    height: 12,
    text: "N"
  }, {
    x: 100,
    y: 24,
    width: 110,
    height: 12,
    text: "D"
  }, {
    x: 460,
    y: 0,
    width: 40,
    height: 12,
    text: "X"
  }]
}, {
  blocks: [{
    x: 210,
    y: 12,
    width: 120,
    height: 12,
    text: "E"
  }, {
    x: 460,
    y: 12,
    width: 40,
    height: 12,
    text: "Y"
  }]
}];
