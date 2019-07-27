






function run_test() {
  run_next_test();
}

const SAMPLE_COUNT = 1000;
const RESOLUTION = 50;
const TIME_PER_SAMPLE = 5;




const TIME_OFFSET = 5;

add_task(function test() {
  let { ThreadNode } = devtools.require("devtools/performance/tree-model");
  let { createTierGraphDataFromFrameNode } = devtools.require("devtools/performance/jit");

  
  let startTime = (SAMPLE_COUNT / 2 * TIME_PER_SAMPLE) - TIME_OFFSET;
  let endTime = (SAMPLE_COUNT * TIME_PER_SAMPLE) - TIME_OFFSET;
  let invertTree = true;

  let root = new ThreadNode(gThread, { invertTree, startTime, endTime });

  equal(root.samples, SAMPLE_COUNT / 2, "root has correct amount of samples");
  equal(root.sampleTimes.length, SAMPLE_COUNT / 2, "root has correct amount of sample times");
  
  equal(root.sampleTimes[0], startTime + TIME_OFFSET, "root recorded first sample time in scope");
  equal(root.sampleTimes[root.sampleTimes.length - 1], endTime, "root recorded last sample time in scope");

  let frame = getFrameNodePath(root, "X");
  let data = createTierGraphDataFromFrameNode(frame, root.sampleTimes, { startTime, endTime, resolution: RESOLUTION });

  let TIME_PER_WINDOW = SAMPLE_COUNT / 2 / RESOLUTION * TIME_PER_SAMPLE;

  for (let i = 0; i < 10; i++) {
    equal(data[i].x, startTime + TIME_OFFSET + (TIME_PER_WINDOW * i), "first window has correct x");
    equal(data[i].ys[0], 0.2, "first window has 2 frames in interpreter");
    equal(data[i].ys[1], 0.2, "first window has 2 frames in baseline");
    equal(data[i].ys[2], 0.2, "first window has 2 frames in ion");
  }
  for (let i = 10; i < 20; i++) {
    equal(data[i].x, startTime + TIME_OFFSET + (TIME_PER_WINDOW * i), "second window has correct x");
    equal(data[i].ys[0], 0, "second window observed no optimizations");
    equal(data[i].ys[1], 0, "second window observed no optimizations");
    equal(data[i].ys[2], 0, "second window observed no optimizations");
  }
  for (let i = 20; i < 30; i++) {
    equal(data[i].x, startTime + TIME_OFFSET + (TIME_PER_WINDOW * i), "third window has correct x");
    equal(data[i].ys[0], 0.3, "third window has 3 frames in interpreter");
    equal(data[i].ys[1], 0, "third window has 0 frames in baseline");
    equal(data[i].ys[2], 0, "third window has 0 frames in ion");
  }
});

let gUniqueStacks = new RecordingUtils.UniqueStacks();

function uniqStr(s) {
  return gUniqueStacks.getOrAddStringIndex(s);
}

const TIER_PATTERNS = [
  
  ["X", "X", "X", "X", "X", "X", "X", "X", "X", "X"],
  
  ["X", "X", "X", "X", "X", "X", "X", "X", "X", "X"],
  
  ["X", "X", "X", "X", "X", "X", "X", "X", "X", "X"],
  
  ["X", "X", "X", "X", "X", "X", "X", "X", "X", "X"],
  
  ["X", "X", "X", "X", "X", "X", "X", "X", "X", "X"],

  
  
  
  ["X", "X", "A", "A", "X_1", "X_2", "X_1", "X_2", "X_0", "X_0"],

  
  
  ["A", "B", "A", "B", "A", "B", "A", "B", "A", "B"],

  
  
  ["X_2 -> Y", "X_2 -> Y", "X_2 -> Y", "X_0", "X_0", "X_0", "A", "A", "A", "A"],

  
  ["X", "X", "X", "X", "X", "X", "X", "X", "X", "X"],
  
  ["X", "X", "X", "X", "X", "X", "X", "X", "X", "X"],
];

function createSample (i, frames) {
  let sample = {};
  sample.time = i * TIME_PER_SAMPLE;
  sample.frames = [{ location: "(root)" }];
  if (i === 0) {
    return sample;
  }
  if (frames) {
    frames.split(" -> ").forEach(frame => sample.frames.push({ location: frame }));
  }
  return sample;
}

let SAMPLES = (function () {
  let samples = [];

  for (let i = 0; i < SAMPLE_COUNT;) {
    let pattern = TIER_PATTERNS[Math.floor(i/100)];
    for (let j = 0; j < pattern.length; j++) {
      samples.push(createSample(i+j, pattern[j]));
    }
    i += 10;
  }

  return samples;
})();

let gThread = RecordingUtils.deflateThread({ samples: SAMPLES, markers: [] }, gUniqueStacks);

let gRawSite1 = {
  line: 12,
  column: 2,
  types: [{
    mirType: uniqStr("Object"),
    site: uniqStr("B (http://foo/bar:10)"),
    typeset: [{
        keyedBy: uniqStr("constructor"),
        name: uniqStr("Foo"),
        location: uniqStr("B (http://foo/bar:10)")
    }, {
        keyedBy: uniqStr("primitive"),
        location: uniqStr("self-hosted")
    }]
  }],
  attempts: {
    schema: {
      outcome: 0,
      strategy: 1
    },
    data: [
      [uniqStr("Failure1"), uniqStr("SomeGetter1")],
      [uniqStr("Failure2"), uniqStr("SomeGetter2")],
      [uniqStr("Inlined"), uniqStr("SomeGetter3")]
    ]
  }
};

function serialize (x) {
  return JSON.parse(JSON.stringify(x));
}

gThread.frameTable.data.forEach((frame) => {
  const LOCATION_SLOT = gThread.frameTable.schema.location;
  const OPTIMIZATIONS_SLOT = gThread.frameTable.schema.optimizations;
  const IMPLEMENTATION_SLOT = gThread.frameTable.schema.implementation;

  let l = gThread.stringTable[frame[LOCATION_SLOT]];
  switch (l) {
  
  
  case "X_0":
    frame[LOCATION_SLOT] = uniqStr("X");
    frame[OPTIMIZATIONS_SLOT] = serialize(gRawSite1);
    frame[IMPLEMENTATION_SLOT] = null;
    break;
  case "X_1":
    frame[LOCATION_SLOT] = uniqStr("X");
    frame[OPTIMIZATIONS_SLOT] = serialize(gRawSite1);
    frame[IMPLEMENTATION_SLOT] = uniqStr("baseline");
    break;
  case "X_2":
    frame[LOCATION_SLOT] = uniqStr("X");
    frame[OPTIMIZATIONS_SLOT] = serialize(gRawSite1);
    frame[IMPLEMENTATION_SLOT] = uniqStr("ion");
    break;
  }
});
