

var BUGNUMBER = 918987;
var summary = 'String.prototype.normalize - part0';

print(BUGNUMBER + ": " + summary);

function test() {
  load('ecma_6/String/normalize-generateddata-input.js');

  for (var test0 of tests_part0) {
    runNormalizeTest(test0);
  }
}

if ("normalize" in String.prototype) {
  
  test();
}

if (typeof reportCompare === "function")
  reportCompare(true, true);
