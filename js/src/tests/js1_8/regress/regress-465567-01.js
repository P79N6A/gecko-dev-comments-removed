





































var BUGNUMBER = 465567;
var summary = 'TM: Weirdness with var, let, multiple assignments';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

expect = '99999';

jit(true);

for (let j = 0; j < 5; ++j) {
  var e;
  e = 9;
  print(actual += '' + e);
  e = 47;
  if (e & 0) {
    let e;
  }
}

jit(false);

reportCompare(expect, actual, summary);
