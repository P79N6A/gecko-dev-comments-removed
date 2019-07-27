







function run_test() {
  run_next_test();
}

add_task(function test() {
  let { ThreadNode } = devtools.require("devtools/performance/tree-model");

  
  
  
  
  
  let startTime = 5;
  let endTime = 18;
  let thread = new ThreadNode(gThread, { startTime, endTime });
  let root = getFrameNodePath(thread, "(root)");

  

  equal(thread.duration, endTime - startTime,
    "The correct duration was calculated for the ThreadNode.");

  equal(root.calls.length, 1,
    "The correct number of child calls were calculated for the root node.");
  ok(getFrameNodePath(root, "A"),
    "The root node's only child call is correct.");

  

  equal(getFrameNodePath(root, "A").calls.length, 2,
    "The correct number of child calls were calculated for the 'A' node.");
  ok(getFrameNodePath(root, "A > B"),
    "The 'A' node has a 'B' child call.");
  ok(getFrameNodePath(root, "A > E"),
    "The 'A' node has a 'E' child call.");

  equal(getFrameNodePath(root, "A > B").calls.length, 1,
    "The correct number of child calls were calculated for the 'A > B' node.");
  ok(getFrameNodePath(root, "A > B > D"),
    "The 'A > B' node's only child call is correct.");

  equal(getFrameNodePath(root, "A > E").calls.length, 1,
    "The correct number of child calls were calculated for the 'A > E' node.");
  ok(getFrameNodePath(root, "A > E > F"),
    "The 'A > E' node's only child call is correct.");

  equal(getFrameNodePath(root, "A > B > D").calls.length, 0,
    "The correct number of child calls were calculated for the 'A > B > D' node.");
  equal(getFrameNodePath(root, "A > E > F").calls.length, 0,
    "The correct number of child calls were calculated for the 'A > E > F' node.");
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
  time: 5 + 6,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "D" }
  ]
}, {
  time: 5 + 6 + 7,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "E" },
    { location: "F" }
  ]
}, {
  time: 5 + 6 + 7 + 8,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "C" },
    { location: "D" }
  ]
}]);
