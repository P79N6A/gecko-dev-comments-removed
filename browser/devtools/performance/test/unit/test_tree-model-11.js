







function run_test() {
  run_next_test();
}

add_task(function () {
  let { ThreadNode } = devtools.require("devtools/performance/tree-model");
  let thread = new ThreadNode(gThread, { startTime: 0, endTime: 50, flattenRecursion: true });

  








  [ 
    [ 100, 0, "(root)", [
      [ 100, 0, "A", [
        [ 100, 50, "B", [
          [ 50, 50, "C"]
        ]]
      ]],
    ]],
  ].forEach(compareFrameInfo(thread));
});

function compareFrameInfo (root, parent) {
  parent = parent || root;
  return function (def) {
    let [total, self, name, children] = def;
    let node = getFrameNodePath(parent, name);
    let data = node.getInfo({ root });
    equal(total, data.totalPercentage, `${name} has correct total percentage: ${data.totalPercentage}`);
    equal(self, data.selfPercentage, `${name} has correct self percentage: ${data.selfPercentage}`);
    if (children) {
      children.forEach(compareFrameInfo(root, node));
    }
  }
}

let gThread = synthesizeProfileForTest([{
  time: 5,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "B" },
    { location: "B" },
    { location: "C" }
  ]
}, {
  time: 10,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "C" }
  ]
}, {
  time: 15,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
    { location: "B" },
    { location: "B" },
  ]
}, {
  time: 20,
  frames: [
    { location: "(root)" },
    { location: "A" },
    { location: "B" },
  ]
}]);
