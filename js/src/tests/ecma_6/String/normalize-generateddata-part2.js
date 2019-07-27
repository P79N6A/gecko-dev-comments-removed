

var BUGNUMBER = 918987;
var summary = 'String.prototype.normalize - part2';

print(BUGNUMBER + ": " + summary);

function test() {
  load('ecma_6/String/normalize-generateddata-input.js');

  for (var test2 of tests_part2) {
    runNormalizeTest(test2);
  }
}

if ("normalize" in String.prototype) {
  
  test();
}

if (typeof reportCompare === "function")
  reportCompare(true, true);
