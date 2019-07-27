"use strict";

const { utils: Cu, interfaces: Ci, classes: Cc } = Components;




function test_sandbox_name() {
  let names = [
    "http://example.com/?" + Math.random(),
    "http://example.org/?" + Math.random()
  ];
  let sandbox = Cu.Sandbox(names);
  let fileName = Cu.evalInSandbox(
    "(new Error()).fileName",
    sandbox,
    "latest" ,
    ""
  );

  for (let name of names) {
    Assert.ok(fileName.indexOf(name) != -1, `Name ${name} appears in ${fileName}`);
  }
};

function run_test() {
  test_sandbox_name();
}
