
















var fs = require('fs');
var vm = require('vm');
var Test = require('./test.js');

var testSuites = {};

function registerTest(name, func) {
  testSuites[name] = func;
}

function registerBotTest(name, func, bots) {
  registerTest(name, bootstrap);

  function bootstrap(test) {
    var callbacks = [];
    for (var i = 0; i != bots.length; ++i)
      callbacks.push(test.spawnBot.bind(test, "", bots[i]));

    test.wait(callbacks, func.bind(test, test));
  }
}

function loadTestFile(filename, doneCallback) {
  var loadTestContext = {
    setTimeout: setTimeout,
    registerTest: registerTest,
    registerBotTest: registerBotTest
  };
  var script = vm.createScript(fs.readFileSync(filename), filename);
  script.runInNewContext(loadTestContext);
  doneCallback();
}

function iterateOverTestFiles(foreachCallback, doneCallback) {
  fs.readdir('test', function (error, list) {
    function iterateNextFile() {
      if (list.length === 0) {
        doneCallback();
      } else {
        var filename = list.pop();
        if (filename[0] === '.' || filename.slice(-3) !== '.js') {
          
          iterateNextFile();
        } else {
          foreachCallback('test/' + filename, iterateNextFile);
        }
      }
    }

    if (error !== null) {
      throw error;
    }
    iterateNextFile();
  });
}

function runTest(testname) {
  if (testname in testSuites) {
    console.log("Running test: " + testname);
    var test = new Test();
    testSuites[testname](test);
  } else {
    console.log("Unknown test: " + testname);
  }
}

function printUsage() {
  console.log('Run as:\n $ '
      + process.argv[0] + ' ' + process.argv[1]
      + ' <testname>');
  console.log('These are the existent ones:');
  for (var testname in testSuites)
    console.log('  ' + testname);
}

function main() {
  
  var testList = process.argv.slice(2);
  if (testList.length === 1)
    runTest(testList[0]);
  else
    printUsage();
}

iterateOverTestFiles(loadTestFile, main);
