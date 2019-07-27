


"use strict";

module.metadata = {
  "stability": "experimental"
};

var { exit, stdout } = require("../system");
var cfxArgs = require("../test/options");
var events = require("../system/events");

function runTests(findAndRunTests) {
  var harness = require("./harness");

  function onDone(tests) {
    stdout.write("\n");
    var total = tests.passed + tests.failed;
    stdout.write(tests.passed + " of " + total + " tests passed.\n");

    events.emit("sdk:test:results", { data: JSON.stringify(tests) });

    if (tests.failed == 0) {
      if (tests.passed === 0)
        stdout.write("No tests were run\n");
      if (!cfxArgs.keepOpen)
        exit(0);
    } else {
      if (cfxArgs.verbose || cfxArgs.parseable)
        printFailedTests(tests, stdout.write);
      if (!cfxArgs.keepOpen)
        exit(1);
    }
  };

  
  
  
  
  require("../timers").setTimeout(_ => harness.runTests({
    findAndRunTests: findAndRunTests,
    iterations: cfxArgs.iterations || 1,
    filter: cfxArgs.filter,
    profileMemory: cfxArgs.profileMemory,
    stopOnError: cfxArgs.stopOnError,
    verbose: cfxArgs.verbose,
    parseable: cfxArgs.parseable,
    print: stdout.write,
    onDone: onDone
  }));
}

function printFailedTests(tests, print) {
  let iterationNumber = 0;
  let singleIteration = (tests.testRuns || []).length == 1;
  let padding = singleIteration ? "" : "  ";

  print("\nThe following tests failed:\n");

  for (let testRun of tests.testRuns) {
    iterationNumber++;

    if (!singleIteration)
      print("  Iteration " + iterationNumber + ":\n");

    for (let test of testRun) {
      if (test.failed > 0) {
        print(padding + "  " + test.name + ": " + test.errors +"\n");
      }
    }
    print("\n");
  }
}

function main() {
  var testsStarted = false;

  if (!testsStarted) {
    testsStarted = true;
    runTests(function findAndRunTests(loader, nextIteration) {
      loader.require("../deprecated/unit-test").findAndRunTests({
        testOutOfProcess: false,
        testInProcess: true,
        stopOnError: cfxArgs.stopOnError,
        filter: cfxArgs.filter,
        onDone: nextIteration
      });
    });
  }
};

if (require.main === module)
  main();

exports.runTestsFromModule = function runTestsFromModule(module) {
  let id = module.id;
  
  let exports = {};
  Object.keys(module.exports).forEach(key => {
    exports[key] = module.exports[key];
  });

  runTests(function findAndRunTests(loader, nextIteration) {
    
    loader.require('../../test').run(exports);

    
    let tests = [];
    for (let name of Object.keys(exports).sort()) {
      tests.push({
        setup: exports.setup,
        teardown: exports.teardown,
        testFunction: exports[name],
        name: id + "." + name
      });
    }

    
    var { TestRunner } = loader.require("../deprecated/unit-test");
    var runner = new TestRunner();
    runner.startMany({
      tests: tests,
      stopOnError: cfxArgs.stopOnError,
      onDone: nextIteration
    });
  });
}
