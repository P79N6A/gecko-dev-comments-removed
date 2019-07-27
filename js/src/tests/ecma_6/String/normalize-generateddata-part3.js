

var BUGNUMBER = 918987;
var summary = 'String.prototype.normalize - part3';

print(BUGNUMBER + ": " + summary);

function test() {
  load('ecma_6/String/normalize-generateddata-input.js');

  for (var test3 of tests_part3) {
    runNormalizeTest(test3);
  }
}

if ("normalize" in String.prototype) {
  
  test();
}

if (typeof reportCompare === "function")
  reportCompare(true, true);
