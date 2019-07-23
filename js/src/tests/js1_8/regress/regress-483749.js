




































var gTestfile = 'regress-483749.js';

var BUGNUMBER = 483749;
var summary = 'Do not assert: !js_IsActiveWithOrBlock(cx, fp->scopeChain, 0)';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

for each (let x in ['']) {
  for (var b = 0; b < 5; ++b) {
    if (b % 5 == 3) {
      with([]) this;
    }
  }
}

jit(false);

reportCompare(expect, actual, summary);
