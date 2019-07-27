






function run_test() {
  run_next_test();
}

add_task(function test() {
  let { ThreadNode } = devtools.require("devtools/performance/tree-model");

  

  let thread = new ThreadNode(gThread, { startTime: 0, endTime: 10 });
  let root = getFrameNodePath(thread, "(root)");

  
  equal(thread.duration, 10,
    "The correct duration was calculated for the ThreadNode.");

  equal(root.calls.length, 1,
    "The correct number of child calls were calculated for the root node.");
  ok(getFrameNodePath(root, "A"),
    "The root node's only child call is correct.");

  

  equal(getFrameNodePath(root, "A").calls.length, 1,
    "The correct number of child calls were calculated for the 'A' node.");
  ok(getFrameNodePath(root, "A > B"),
    "The 'A' node's only child call is correct.");

  equal(getFrameNodePath(root, "A > B").calls.length, 1,
    "The correct number of child calls were calculated for the 'A > B' node.");
  ok(getFrameNodePath(root, "A > B > C"),
    "The 'A > B' node's only child call is correct.");

  equal(getFrameNodePath(root, "A > B > C").calls.length, 0,
    "The correct number of child calls were calculated for the 'A > B > C' node.");
});

let gThread = synthesizeProfileForTest([{
  time: 5,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "C" }
  ]
}, {
  time: null,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "D" }
  ]
}]);
