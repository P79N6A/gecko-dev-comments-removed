






var BUGNUMBER = 418730;
var summary = 'export * should not halt script';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
for (var i = 0; i < 60; ++i)
  this["v" + i] = true;

expect = 'PASS';
actual = 'FAIL';

try {
  print("GO");
  export *;
  print("PASS (1)");
} catch(e) {
  print("PASS (2)");
}

actual = 'PASS';

reportCompare(expect, actual, summary);
