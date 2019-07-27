






"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-console-output-regexp.html";

let dateNow = Date.now();

let inputTests = [
  
  {
    input: "/foo/igym",
    output: "/foo/gimy",
    printOutput: "Error: source called",
    inspectable: true,
  },
];

function test() {
  requestLongerTimeout(2);
  Task.spawn(function*() {
    let {tab} = yield loadTab(TEST_URI);
    let hud = yield openConsole(tab);
    return checkOutputForInputs(hud, inputTests);
  }).then(finishUp);
}

function finishUp() {
  inputTests = dateNow = null;
  finishTest();
}
