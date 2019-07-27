

var BUGNUMBER = 918987;
var summary = 'String.prototype.normalize - part1';

print(BUGNUMBER + ": " + summary);

function test() {
  load('ecma_6/String/normalize-generateddata-input.js');

  for (var test1 of tests_part1) {
    runNormalizeTest(test1);
  }
}

if ("normalize" in String.prototype) {
  
  test();
}

if (typeof reportCompare === "function")
  reportCompare(true, true);
